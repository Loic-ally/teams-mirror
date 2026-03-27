#include "sender.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <cerrno>

extern "C" {
    #include <sys/socket.h>
    #include <sys/types.h>
}

namespace server::network {

std::int64_t
Sender::sendBytes(std::int32_t socketFd, const char *buffer, std::size_t bufferSize)
{
	ssize_t sendResult = -1;
	do {
		sendResult = ::send(socketFd, buffer, bufferSize, 0);
	} while (sendResult < 0 && errno == EINTR);
	if (sendResult < 0)
		throw SocketSendException(errno);
	return static_cast<std::int64_t>(sendResult);
}

bool
Sender::flushClientData(ClientManager &clientManager, std::int32_t clientFd)
{
	if (!clientManager.hasPendingWrite(clientFd))
		return true;
	const char *writeData = clientManager.getQueuedData(clientFd);
	const std::size_t writeSize = clientManager.getQueuedSize(clientFd);
	if (writeData == nullptr || writeSize == 0)
		return true;
	const std::int64_t bytesSent = sendBytes(clientFd, writeData, writeSize);
	if (bytesSent <= 0)
		return false;
	clientManager.consumeQueuedData(clientFd, static_cast<std::size_t>(bytesSent));
	return true;
}

} // namespace server::network
