#include "server_manager.hpp"
#include "server_exceptions.hpp"

#include <cerrno>

extern "C" {
    #include <arpa/inet.h>
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
