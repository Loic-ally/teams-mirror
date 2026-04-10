#include "sender.hpp"
#include "common/utils/Socket.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <cerrno>
#include <string_view>

namespace server::network {

std::int64_t
Sender::sendBytes(std::int32_t socketFd, const std::string_view payload)
{
	try {
		const utils::Socket socket(socketFd);
		return static_cast<std::int64_t>(socket.write(std::string(payload)));
	} catch (const utils::SocketException &exception) {
		throw SocketSendException(exception.errorNumber());
	}
}

bool
Sender::flushClientData(ClientManager &clientManager, std::int32_t clientFd)
{
	if (!clientManager.hasPendingWrite(clientFd))
		return true;
	const std::string_view queuedData = clientManager.getQueuedData(clientFd);
	if (queuedData.empty())
		return true;
	const std::int64_t bytesSent = sendBytes(clientFd, queuedData);
	if (bytesSent <= 0)
		return false;
	clientManager.consumeQueuedData(clientFd, static_cast<std::size_t>(bytesSent));
	return true;
}

} // namespace server::network
