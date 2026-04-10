#include "reciever.hpp"
#include "common/utils/Socket.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <cerrno>
#include <string>

namespace server::network {

std::string
Reciever::recvBytes(std::int32_t socketFd, const std::size_t bufferSize)
{
	try {
		const utils::Socket socket(socketFd);
		return socket.read(bufferSize);
	} catch (const utils::SocketException &exception) {
		throw SocketReceiveException(exception.errorNumber());
	}
}

bool
Reciever::readClientData(ClientManager &clientManager, std::int32_t clientFd)
{
	constexpr std::size_t READ_BUFFER_SIZE = 4096;
	const std::string data = recvBytes(clientFd, READ_BUFFER_SIZE);
	if (data.empty())
		return false;
	clientManager.appendToIncomingBuffer(clientFd, data);
	return true;
}

} // namespace server::network
