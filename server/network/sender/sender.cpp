#include "sender.hpp"
#include "common/utils/Socket.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <cerrno>
#include <string>

namespace server::network {

std::int64_t
Sender::sendBytes(std::int32_t socketFd, const char *buffer, std::size_t bufferSize)
{
	try {
		const utils::Socket socket(socketFd);
		if (buffer == nullptr || bufferSize == 0)
			return 0;
		const std::string payload(buffer, bufferSize);
		return static_cast<std::int64_t>(socket.write(payload));
	} catch (const utils::SocketException &exception) {
		throw SocketSendException(exception.errorNumber());
	}
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
