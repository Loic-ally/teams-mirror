#include "server_manager.hpp"
#include "common/utils/Socket.hpp"
#include "server/commands/command_dispatcher.hpp"
#include "exceptions/server_exceptions.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/logger/server_logger.hpp"
#include "database/load/load.hpp"
#include "database/save/save.hpp"
#include "network/reciever/reciever.hpp"
#include "network/sender/sender.hpp"

#include <csignal>
#include <cerrno>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <vector>

extern "C" {
    #include <arpa/inet.h>
    #include <poll.h>
    #include <sys/socket.h>
}

namespace server {

namespace {

using PollFdList = std::vector<struct pollfd>;
using ClientSocketMap = commands::ClientSockets;
using AuthenticatedUserByFd = commands::AuthenticatedUserByFd;

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
appendClientPollFd(PollFdList &pollFds, const std::int32_t clientFd)
{
    struct pollfd clientPollFd {};
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    pollFds.push_back(clientPollFd);
}

bool
handleReadableEvent(ClientManager &clientManager, const std::int32_t clientFd)
{
    try {
        return network::Reciever::readClientData(clientManager, clientFd);
    } catch (const SocketReceiveException &) {
        return false;
    }
}

bool
handleWritableEvent(ClientManager &clientManager, const std::int32_t clientFd)
{
    if (!clientManager.hasPendingWrite(clientFd)) {
        return true;
    }
    try {
        return network::Sender::flushClientData(clientManager, clientFd);
    } catch (const SocketSendException &) {
        return false;
    }
}

bool
shouldKeepClientConnected(
    ClientManager &clientManager,
    const std::int32_t clientFd,
    const short clientEvents,
    std::vector<myteams::User> &users,
    const ClientSocketMap &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd)
{
    if ((clientEvents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        return false;
    }
    if ((clientEvents & POLLIN) != 0) {
        if (!handleReadableEvent(clientManager, clientFd)) {
            return false;
        }
        commands::processClientIncomingPackets(
            clientManager,
            clientFd,
            users,
            clientSockets,
            authenticatedUsersByFd);
    }
    if ((clientEvents & POLLOUT) != 0 && !handleWritableEvent(clientManager, clientFd)) {
        return false;
    }
    return true;
}

void
refreshClientPollInterest(
    struct pollfd &clientPollFd,
    const ClientManager &clientManager,
    const std::int32_t clientFd)
{
    clientPollFd.events = clientManager.hasPendingWrite(clientFd)
        ? static_cast<short>(POLLIN | POLLOUT)
        : POLLIN;
    clientPollFd.revents = 0;
}

void
removeClientAt(
    PollFdList &pollFds,
    const std::size_t index,
    ClientSocketMap &clientSockets,
    ClientManager &clientManager,
    std::vector<myteams::User> &users,
    AuthenticatedUserByFd &authenticatedUsersByFd)
{
    const std::int32_t clientFd = pollFds[index].fd;
    commands::handleClientDisconnection(
        clientManager,
        clientFd,
        users,
        clientSockets,
        authenticatedUsersByFd);
    clientSockets.erase(clientFd);
    clientManager.removeClient(clientFd);
    pollFds.erase(pollFds.begin()
        + static_cast<std::vector<struct pollfd>::difference_type>(index));
}

} // namespace

std::atomic<bool> ServerManager::_isRunning {true};

ServerManager::ServerManager(const std::uint16_t port)
    : _listenSocket(nullptr), _port(port)
{
}

ServerManager::~ServerManager() noexcept = default;

ServerManager::ServerManager(ServerManager &&other) noexcept
    : _listenSocket(std::move(other._listenSocket)), _port(other._port)
{
}

ServerManager &
ServerManager::operator=(ServerManager &&other) noexcept
{
    if (this != &other) {
        _listenSocket = std::move(other._listenSocket);
        _port = other._port;
    }
    return *this;
}

std::int32_t
ServerManager::pollSockets(std::vector<struct pollfd> &pollFds, const std::int32_t timeoutMs)
{
    std::int32_t pollResult = -1;
    do {
        pollResult = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), timeoutMs);
    } while (pollResult < 0 && errno == EINTR && _isRunning.load());
    if (pollResult < 0) {
        if (errno == EINTR && !_isRunning.load()) {
            return 0;
        }
        throw SocketPollException(errno);
    }
    return pollResult;
}

void
ServerManager::handleSignal(const std::int32_t signal) noexcept
{
    if (signal == SIGINT) {
        _isRunning.store(false);
    }
}

void
ServerManager::installSignalHandler()
{
    if (std::signal(SIGINT, &ServerManager::handleSignal) == SIG_ERR) {
        throw ServerException("signal(SIGINT) registration failed");
    }
}

void
ServerManager::initializeTcpListener(const std::int32_t backlog)
{
    if (_listenSocket != nullptr) {
        throw SocketAlreadyInitializedException();
    }
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
    if (_listenSocket == nullptr) {
        throw SocketNotInitializedException();
    }
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
        if (!_isRunning.load()) {
            break;
        }
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
    if (!databaseSaver.save(_users, _teams)) {
        std::cerr << "Server state could not be fully saved." << std::endl;
    }
}

std::int32_t
ServerManager::getListenFd() const noexcept
{
    if (_listenSocket == nullptr) {
        return -1;
    }
    return _listenSocket->getFd();
}

std::uint16_t
ServerManager::getPort() const noexcept
{
    return _port;
}

} // namespace server

