#include "server_manager.hpp"
#include "common/protocol.hpp"
#include "common/utils/Socket.hpp"
#include "exceptions/server_exceptions.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/logger/server_logger.hpp"
#include "database/load/load.hpp"
#include "database/save/save.hpp"
#include "network/reciever/reciever.hpp"
#include "network/sender/sender.hpp"

#include <algorithm>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

extern "C" {
    #include <arpa/inet.h>
	#include <poll.h>
    #include <sys/socket.h>
}

namespace server {

namespace {

using PollFdList = std::vector<struct pollfd>;
using ClientSocketMap = std::unordered_map<std::int32_t, std::unique_ptr<utils::Socket>>;
using AuthenticatedUserByFd = std::unordered_map<std::int32_t, std::string>;

void
copyPaddedString(char *destination, std::size_t size, std::string_view source)
{
	if (destination == nullptr || size == 0)
		return;
	const std::size_t copiedLength = source.size() < (size - 1) ? source.size() : (size - 1);
	std::memcpy(destination, source.data(), copiedLength);
	destination[copiedLength] = '\0';
}

std::string
buildPacket(std::uint16_t code, const void *payload = nullptr, std::uint16_t payloadSize = 0)
{
	const myteams::PacketHeader header { code, payloadSize };
	std::string packet(sizeof(header) + payloadSize, '\0');
	std::memcpy(packet.data(), &header, sizeof(header));
	if (payload != nullptr && payloadSize > 0)
		std::memcpy(packet.data() + sizeof(header), payload, payloadSize);
	return packet;
}

void
queuePacket(ClientManager &clientManager, std::int32_t clientFd, const std::string &packet)
{
	clientManager.queueDataToSend(clientFd, packet.data(), packet.size());
}

void
queueStatus(ClientManager &clientManager, std::int32_t clientFd, myteams::StatusCode status)
{
	queuePacket(clientManager, clientFd, buildPacket(static_cast<std::uint16_t>(status)));
}

void
broadcastEvent(
	ClientManager &clientManager,
	const ClientSocketMap &clientSockets,
	const std::string &packet,
	std::int32_t excludedClientFd = -1)
{
	for (const auto &[socketFd, socket] : clientSockets) {
		(void)socket;
		if (socketFd == excludedClientFd)
			continue;
		clientManager.queueDataToSend(socketFd, packet.data(), packet.size());
	}
}

std::string
generateUuid()
{
	static thread_local std::mt19937 randomEngine(std::random_device {}());
	static constexpr char HEX_DIGITS[] = "0123456789abcdef";
	static constexpr std::size_t UUID_CHAR_COUNT = 36;
	std::string uuid(UUID_CHAR_COUNT, '\0');
	std::uniform_int_distribution<int> distribution(0, 15);

	for (std::size_t index = 0; index < UUID_CHAR_COUNT; ++index) {
		if (index == 8 || index == 13 || index == 18 || index == 23) {
			uuid[index] = '-';
			continue;
		}
		uuid[index] = HEX_DIGITS[distribution(randomEngine)];
	}
	return uuid;
}

myteams::User *
findUserByName(std::vector<myteams::User> &users, std::string_view userName)
{
	const auto it = std::find_if(users.begin(), users.end(),
		[userName](const myteams::User &user) { return user.getName() == userName; });
	if (it == users.end())
		return nullptr;
	return &(*it);
}

myteams::User *
findUserByUuid(std::vector<myteams::User> &users, std::string_view userUuid)
{
	const auto it = std::find_if(users.begin(), users.end(),
		[userUuid](const myteams::User &user) { return user.getUuid() == userUuid; });
	if (it == users.end())
		return nullptr;
	return &(*it);
}

bool
extractFixedString(const char *rawData, std::size_t rawSize, std::string &outString)
{
	if (rawData == nullptr || rawSize == 0)
		return false;
	const void *nullTerminator = std::memchr(rawData, '\0', rawSize);
	if (nullTerminator == nullptr)
		return false;
	const auto *end = static_cast<const char *>(nullTerminator);
	outString.assign(rawData, static_cast<std::size_t>(end - rawData));
	return true;
}

std::string
buildUserConnectionEventPacket(myteams::StatusCode eventCode, std::string_view userUuid, std::string_view userName)
{
	myteams::PayloadEvtUserConnection payload {};
	copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), userUuid);
	copyPaddedString(payload.user_name, sizeof(payload.user_name), userName);
	return buildPacket(static_cast<std::uint16_t>(eventCode), &payload, sizeof(payload));
}

void
handleLoginCommand(
	ClientManager &clientManager,
	std::int32_t clientFd,
	const char *payloadData,
	std::uint16_t payloadSize,
	std::vector<myteams::User> &users,
	const ClientSocketMap &clientSockets,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	if (payloadSize != sizeof(myteams::PayloadReqLogin)) {
		queueStatus(clientManager, clientFd, myteams::ERR_BAD_REQUEST);
		return;
	}
	if (authenticatedUsersByFd.find(clientFd) != authenticatedUsersByFd.end()) {
		queueStatus(clientManager, clientFd, myteams::ERR_ALREADY_EXIST);
		return;
	}

	myteams::PayloadReqLogin payload {};
	std::memcpy(&payload, payloadData, sizeof(payload));
	std::string requestedUserName;
	if (!extractFixedString(payload.user_name, sizeof(payload.user_name), requestedUserName)
		|| requestedUserName.empty()) {
		queueStatus(clientManager, clientFd, myteams::ERR_BAD_REQUEST);
		return;
	}

	myteams::User *user = findUserByName(users, requestedUserName);
	if (user == nullptr) {
		std::string userUuid = generateUuid();
		while (findUserByUuid(users, userUuid) != nullptr)
			userUuid = generateUuid();
		users.emplace_back(userUuid, requestedUserName, true);
		user = &users.back();
		(void)ServerLogger::logUserCreated(user->getUuid(), user->getName());
	} else {
		if (user->isLoggedIn()) {
			queueStatus(clientManager, clientFd, myteams::ERR_ALREADY_EXIST);
			return;
		}
		user->setLoggedIn(true);
	}

	authenticatedUsersByFd[clientFd] = std::string(user->getUuid());
	(void)ServerLogger::logUserLoggedIn(user->getUuid());
	queueStatus(clientManager, clientFd, myteams::RPL_OK);

	const std::string eventPacket =
		buildUserConnectionEventPacket(myteams::EVT_LOGGED_IN, user->getUuid(), user->getName());
	broadcastEvent(clientManager, clientSockets, eventPacket);
}

void
handleLogoutCommand(
	ClientManager &clientManager,
	std::int32_t clientFd,
	std::uint16_t payloadSize,
	std::vector<myteams::User> &users,
	const ClientSocketMap &clientSockets,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	if (payloadSize != 0) {
		queueStatus(clientManager, clientFd, myteams::ERR_BAD_REQUEST);
		return;
	}

	const auto authenticatedUserIt = authenticatedUsersByFd.find(clientFd);
	if (authenticatedUserIt == authenticatedUsersByFd.end()) {
		queueStatus(clientManager, clientFd, myteams::ERR_UNAUTHORIZED);
		return;
	}

	myteams::User *user = findUserByUuid(users, authenticatedUserIt->second);
	authenticatedUsersByFd.erase(authenticatedUserIt);
	if (user == nullptr) {
		queueStatus(clientManager, clientFd, myteams::ERR_UNAUTHORIZED);
		return;
	}

	user->setLoggedIn(false);
	(void)ServerLogger::logUserLoggedOut(user->getUuid());
	queueStatus(clientManager, clientFd, myteams::RPL_OK);

	const std::string eventPacket =
		buildUserConnectionEventPacket(myteams::EVT_LOGGED_OUT, user->getUuid(), user->getName());
	broadcastEvent(clientManager, clientSockets, eventPacket);
}

void
processClientIncomingPackets(
	ClientManager &clientManager,
	std::int32_t clientFd,
	std::vector<myteams::User> &users,
	const ClientSocketMap &clientSockets,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	for (;;) {
		const std::string &incomingBuffer = clientManager.getIncomingBuffer(clientFd);
		if (incomingBuffer.size() < sizeof(myteams::PacketHeader))
			return;

		myteams::PacketHeader header {};
		std::memcpy(&header, incomingBuffer.data(), sizeof(header));
		const std::size_t packetSize = sizeof(header) + header.payload_size;
		if (incomingBuffer.size() < packetSize)
			return;

		const char *payloadData = incomingBuffer.data() + sizeof(header);
		switch (header.code) {
		case myteams::CMD_LOGIN:
			handleLoginCommand(
				clientManager,
				clientFd,
				payloadData,
				header.payload_size,
				users,
				clientSockets,
				authenticatedUsersByFd);
			break;
		case myteams::CMD_LOGOUT:
			handleLogoutCommand(
				clientManager,
				clientFd,
				header.payload_size,
				users,
				clientSockets,
				authenticatedUsersByFd);
			break;
		case myteams::CMD_INFO:
			if (header.payload_size == 0)
				queueStatus(clientManager, clientFd, myteams::RPL_OK);
			else
				queueStatus(clientManager, clientFd, myteams::ERR_BAD_REQUEST);
			break;
		default:
			queueStatus(clientManager, clientFd, myteams::ERR_BAD_REQUEST);
			break;
		}
		clientManager.consumeIncomingBuffer(clientFd, packetSize);
	}
}

void
handleClientDisconnection(
	ClientManager &clientManager,
	std::int32_t clientFd,
	std::vector<myteams::User> &users,
	const ClientSocketMap &clientSockets,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	const auto authenticatedUserIt = authenticatedUsersByFd.find(clientFd);
	if (authenticatedUserIt == authenticatedUsersByFd.end())
		return;

	myteams::User *user = findUserByUuid(users, authenticatedUserIt->second);
	authenticatedUsersByFd.erase(authenticatedUserIt);
	if (user == nullptr)
		return;

	user->setLoggedIn(false);
	(void)ServerLogger::logUserLoggedOut(user->getUuid());
	const std::string eventPacket =
		buildUserConnectionEventPacket(myteams::EVT_LOGGED_OUT, user->getUuid(), user->getName());
	broadcastEvent(clientManager, clientSockets, eventPacket, clientFd);
}

std::unique_ptr<utils::Socket>
acceptClientSocket(const utils::Socket &listenSocket)
{
	try {
		auto accepted = listenSocket.accept();
		return std::move(accepted.first);
	} catch (const utils::SocketException &exception) {
		throw SocketAcceptException(exception.errorNumber());
	}
}

void
appendClientPollFd(PollFdList &pollFds, std::int32_t clientFd)
{
	struct pollfd clientPollFd {};
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	pollFds.push_back(clientPollFd);
}

bool
handleReadableEvent(ClientManager &clientManager, std::int32_t clientFd)
{
	try {
		return network::Reciever::readClientData(clientManager, clientFd);
	} catch (const SocketReceiveException &) {
		return false;
	}
}

bool
handleWritableEvent(ClientManager &clientManager, std::int32_t clientFd)
{
	if (!clientManager.hasPendingWrite(clientFd))
		return true;
	try {
		return network::Sender::flushClientData(clientManager, clientFd);
	} catch (const SocketSendException &) {
		return false;
	}
}

bool
shouldKeepClientConnected(
	ClientManager &clientManager,
	std::int32_t clientFd,
	short clientEvents,
	std::vector<myteams::User> &users,
	const ClientSocketMap &clientSockets,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	if ((clientEvents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
		return false;
	if ((clientEvents & POLLIN) != 0) {
		if (!handleReadableEvent(clientManager, clientFd))
			return false;
		processClientIncomingPackets(clientManager, clientFd, users, clientSockets, authenticatedUsersByFd);
	}
	if ((clientEvents & POLLOUT) != 0 && !handleWritableEvent(clientManager, clientFd))
		return false;
	return true;
}

void
refreshClientPollInterest(struct pollfd &clientPollFd, const ClientManager &clientManager,
	std::int32_t clientFd)
{
	clientPollFd.events = clientManager.hasPendingWrite(clientFd)
		? static_cast<short>(POLLIN | POLLOUT)
		: POLLIN;
	clientPollFd.revents = 0;
}

void
removeClientAt(PollFdList &pollFds, std::size_t index, ClientSocketMap &clientSockets,
	ClientManager &clientManager, std::vector<myteams::User> &users,
	AuthenticatedUserByFd &authenticatedUsersByFd)
{
	const std::int32_t clientFd = pollFds[index].fd;
	handleClientDisconnection(clientManager, clientFd, users, clientSockets, authenticatedUsersByFd);
	clientSockets.erase(clientFd);
	clientManager.removeClient(clientFd);
	pollFds.erase(pollFds.begin()
		+ static_cast<std::vector<struct pollfd>::difference_type>(index));
}

} // namespace

std::atomic<bool> ServerManager::_isRunning {true};

ServerManager::ServerManager(std::uint16_t port)
	: _listenSocket(nullptr), _port(port)
{
}

ServerManager::~ServerManager() noexcept = default;

ServerManager::ServerManager(ServerManager &&other) noexcept
	: _listenSocket(std::move(other._listenSocket)), _port(other._port)
{
}

ServerManager&
ServerManager::operator=(ServerManager &&other) noexcept
{
	if (this != &other) {
		_listenSocket = std::move(other._listenSocket);
		_port = other._port;
	}
	return *this;
}

std::int32_t
ServerManager::pollSockets(std::vector<struct pollfd> &pollFds, std::int32_t timeoutMs)
{
	std::int32_t pollResult = -1;
	do {
		pollResult = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), timeoutMs);
	} while (pollResult < 0 && errno == EINTR && _isRunning.load());
	if (pollResult < 0) {
		if (errno == EINTR && !_isRunning.load())
			return 0;
		throw SocketPollException(errno);
	}
	return pollResult;
}

void
ServerManager::handleSignal(std::int32_t signal) noexcept
{
	if (signal == SIGINT)
		_isRunning.store(false);
}

void
ServerManager::installSignalHandler()
{
	if (std::signal(SIGINT, &ServerManager::handleSignal) == SIG_ERR)
		throw ServerException("signal(SIGINT) registration failed");
}

void
ServerManager::initializeTcpListener(std::int32_t backlog)
{
	if (_listenSocket != nullptr)
		throw SocketAlreadyInitializedException();
	std::unique_ptr<utils::Socket> listenSocket;
	try {
		listenSocket = std::make_unique<utils::Socket>(AF_INET, SOCK_STREAM, 0);
	} catch (const utils::SocketException &exception) {
		throw SocketCreationException(exception.errorNumber());
	}
	try {
		listenSocket->setReuseAddress();
	} catch (const utils::SocketException &exception) {
		throw SocketOptionException(exception.errorNumber());
	}
	sockaddr_in serverAddr {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(_port);
	try {
		listenSocket->bind(*reinterpret_cast<const sockaddr *>(&serverAddr));
	} catch (const utils::SocketException &exception) {
		throw SocketBindException(exception.errorNumber());
	}
	try {
		listenSocket->listen(backlog);
	} catch (const utils::SocketException &exception) {
		throw SocketListenException(exception.errorNumber());
	}
	_listenSocket = std::move(listenSocket);
}

void
ServerManager::runPollLoop()
{
	if (_listenSocket == nullptr)
		throw SocketNotInitializedException();
	database::DatabaseLoader databaseLoader;
	(void)databaseLoader.load(_users, _teams);
	installSignalHandler();
	_isRunning.store(true);
	PollFdList pollFds;
	ClientSocketMap clientSockets;
	AuthenticatedUserByFd authenticatedUsersByFd;
	ClientManager clientManager;
	for (myteams::User &user : _users) {
		user.setLoggedIn(false);
		(void)ServerLogger::logUserLoaded(user.getUuid(), user.getName());
	}
	const std::int32_t listenFd = _listenSocket->getFd();
	struct pollfd listenPollFd {};
	listenPollFd.fd = listenFd;
	listenPollFd.events = POLLIN;
	pollFds.push_back(listenPollFd);
	while (_isRunning.load()) {
		(void)pollSockets(pollFds, -1);
		if (!_isRunning.load())
			break;
		for (std::size_t index = 0; index < pollFds.size();) {
			struct pollfd &currentPollFd = pollFds[index];
			const std::int32_t currentFd = currentPollFd.fd;
			const short currentEvents = currentPollFd.revents;
			if (currentEvents == 0) {
				++index;
				continue;
			}
			if (currentFd == listenFd) {
				if ((currentEvents & POLLIN) != 0) {
					std::unique_ptr<utils::Socket> acceptedSocket = acceptClientSocket(*_listenSocket);
					const std::int32_t clientFd = acceptedSocket->getFd();
					clientSockets.emplace(clientFd, std::move(acceptedSocket));
					clientManager.addClient(clientFd);
					appendClientPollFd(pollFds, clientFd);
				}
				currentPollFd.revents = 0;
				++index;
				continue;
			}
			if (shouldKeepClientConnected(
				clientManager,
				currentFd,
				currentEvents,
				_users,
				clientSockets,
				authenticatedUsersByFd)) {
				refreshClientPollInterest(currentPollFd, clientManager, currentFd);
				++index;
				continue;
			}
			removeClientAt(
				pollFds,
				index,
				clientSockets,
				clientManager,
				_users,
				authenticatedUsersByFd);
		}
	}
	clientSockets.clear();
	database::DatabaseSaver databaseSaver;
	if (!databaseSaver.save(_users, _teams))
		std::cerr << "Server state could not be fully saved." << std::endl;
}

std::int32_t
ServerManager::getListenFd() const noexcept
{
	if (_listenSocket == nullptr)
		return -1;
	return _listenSocket->getFd();
}

std::uint16_t
ServerManager::getPort() const noexcept
{
	return _port;
}

} // namespace server
