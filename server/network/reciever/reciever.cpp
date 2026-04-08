#include "reciever.hpp"
#include "common/utils/Socket.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <array>
#include <cerrno>
#include <cstring>

namespace server::network {

std::int64_t
Reciever::recvBytes(std::int32_t socketFd, char *buffer, std::size_t bufferSize)
{
	try {
		const utils::Socket socket(socketFd);
		const std::string data = socket.read(bufferSize);
		if (!data.empty())
			std::memcpy(buffer, data.data(), data.size());
		return static_cast<std::int64_t>(data.size());
	} catch (const utils::SocketException &exception) {
		throw SocketReceiveException(exception.errorNumber());
	}
}

bool
Reciever::readClientData(ClientManager &clientManager, std::int32_t clientFd)
{
	constexpr std::size_t READ_BUFFER_SIZE = 4096;
	std::array<char, READ_BUFFER_SIZE> readBuffer {};
	const std::int64_t bytesRead = recvBytes(clientFd, readBuffer.data(), readBuffer.size());
	if (bytesRead <= 0)
		return false;
	clientManager.appendToIncomingBuffer(clientFd, readBuffer.data(), static_cast<std::size_t>(bytesRead));
	return true;
}

} // namespace server::network
