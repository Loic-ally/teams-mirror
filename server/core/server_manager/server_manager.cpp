#include "server_manager.hpp"
#include "exceptions/server_exceptions.hpp"
#include "core/client_manager/client_manager.hpp"
#include "network/reciever/reciever.hpp"
#include "network/sender/sender.hpp"

#include <cerrno>
#include <vector>

extern "C" {
    #include <arpa/inet.h>
	#include <poll.h>
	#include <sys/types.h>
    #include <sys/socket.h>
    #include <unistd.h>
}

namespace server {

ServerManager::ServerManager(std::uint16_t port)
	: _listenFd(-1), _port(port)
{
}

ServerManager::~ServerManager() noexcept
{
	closeSocket(_listenFd);
}

ServerManager::ServerManager(ServerManager &&other) noexcept
	: _listenFd(other._listenFd), _port(other._port)
{
	other._listenFd = -1;
}

ServerManager&
ServerManager::operator=(ServerManager &&other) noexcept
{
	if (this != &other) {
		closeSocket(_listenFd);
		_listenFd = other._listenFd;
		_port = other._port;
		other._listenFd = -1;
	}
	return *this;
}

void
ServerManager::closeSocket(std::int32_t &fd) noexcept
{
	if (fd < 0)
		return;
	(void)::close(fd);
	fd = -1;
}

std::int32_t
ServerManager::createTcpSocket()
{
	const std::int32_t socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		throw SocketCreationException(errno);
	return socketFd;
}

void
ServerManager::setReuseAddress(std::int32_t socketFd)
{
	const std::int32_t reuseAddr = 1;
	if (::setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0)
		throw SocketOptionException(errno);
}

std::uint32_t
ServerManager::toNetworkAddress(std::uint32_t hostAddress)
{
	return htonl(hostAddress);
}

std::uint16_t
ServerManager::toNetworkPort(std::uint16_t hostPort)
{
	return htons(hostPort);
}

std::int32_t
ServerManager::pollSockets(std::vector<struct pollfd> &pollFds, std::int32_t timeoutMs)
{
	std::int32_t pollResult = -1;
	do {
		pollResult = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), timeoutMs);
	} while (pollResult < 0 && errno == EINTR);
	if (pollResult < 0)
		throw SocketPollException(errno);
	return pollResult;
}

std::int32_t
ServerManager::acceptClient(std::int32_t socketFd)
{
	std::int32_t acceptedFd = -1;
	do {
		acceptedFd = ::accept(socketFd, nullptr, nullptr);
	} while (acceptedFd < 0 && errno == EINTR);
	if (acceptedFd < 0)
		throw SocketAcceptException(errno);
	return acceptedFd;
}

void
ServerManager::bindSocket(std::int32_t socketFd) const
{
	sockaddr_in serverAddr {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = toNetworkAddress(INADDR_ANY);
	serverAddr.sin_port = toNetworkPort(_port);
	if (::bind(socketFd, reinterpret_cast<const sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0)
		throw SocketBindException(errno);
}

void
ServerManager::listenSocket(std::int32_t socketFd, std::int32_t backlog)
{
	if (::listen(socketFd, backlog) < 0)
		throw SocketListenException(errno);
}

void
ServerManager::initializeTcpListener(std::int32_t backlog)
{
	if (_listenFd >= 0)
		throw SocketAlreadyInitializedException();
	std::int32_t socketFd = createTcpSocket();
	try {
		setReuseAddress(socketFd);
		bindSocket(socketFd);
		listenSocket(socketFd, backlog);
		_listenFd = socketFd;
	} catch (...) {
		closeSocket(socketFd);
		throw;
	}
}

void
ServerManager::runPollLoop()
{
	if (_listenFd < 0)
		throw SocketNotInitializedException();
	std::vector<struct pollfd> pollFds;
	ClientManager clientManager;
	bool running = true;
	struct pollfd listenPollFd {};
	listenPollFd.fd = _listenFd;
	listenPollFd.events = POLLIN;
	pollFds.push_back(listenPollFd);
	while (running) {
		(void)pollSockets(pollFds, -1);
		for (std::size_t index = 0; index < pollFds.size();) {
			const std::int32_t currentFd = pollFds[index].fd;
			const short currentEvents = pollFds[index].revents;

			if (currentEvents == 0) {
				++index;
				continue;
			}
			if (currentFd == _listenFd) {
				if ((currentEvents & POLLIN) != 0) {
					const std::int32_t clientFd = acceptClient(_listenFd);
					struct pollfd clientPollFd {};
					clientPollFd.fd = clientFd;
					clientPollFd.events = POLLIN;
					pollFds.push_back(clientPollFd);
					clientManager.addClient(clientFd);
				}
				pollFds[index].revents = 0;
				++index;
				continue;
			}
			bool removeCurrentClient = false;
			if ((currentEvents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
				removeCurrentClient = true;
			if (!removeCurrentClient && (currentEvents & POLLIN) != 0) {
				try {
					if (!network::Reciever::readClientData(clientManager, currentFd)) {
						removeCurrentClient = true;
					}
				} catch (const SocketReceiveException &) {
					removeCurrentClient = true;
				}
			}
			if (!removeCurrentClient && (currentEvents & POLLOUT) != 0) {
				if (clientManager.hasPendingWrite(currentFd)) {
					try {
						if (!network::Sender::flushClientData(clientManager, currentFd)) {
							removeCurrentClient = true;
						}
					} catch (const SocketSendException &) {
						removeCurrentClient = true;
					}
				}
			}
			if (!removeCurrentClient) {
				pollFds[index].events = clientManager.hasPendingWrite(currentFd)
					? static_cast<short>(POLLIN | POLLOUT)
					: POLLIN;
			}
			pollFds[index].revents = 0;
			if (!removeCurrentClient) {
				++index;
				continue;
			}
			closeSocket(pollFds[index].fd);
			clientManager.removeClient(currentFd);
			pollFds.erase(pollFds.begin() + static_cast<std::vector<struct pollfd>::difference_type>(index));
		}
	}
}

std::int32_t
ServerManager::getListenFd() const noexcept
{
	return _listenFd;
}

std::uint16_t
ServerManager::getPort() const noexcept
{
	return _port;
}

} // namespace server
