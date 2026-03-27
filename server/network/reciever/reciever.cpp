#include "reciever.hpp"
#include "core/client_manager/client_manager.hpp"
#include "core/exceptions/server_exceptions.hpp"

#include <array>
#include <cerrno>

extern "C" {
    #include <sys/socket.h>
    #include <sys/types.h>
}

namespace server::network {

std::int64_t
Reciever::recvBytes(std::int32_t socketFd, char *buffer, std::size_t bufferSize)
{
	ssize_t recvResult = -1;
	do {
		recvResult = ::recv(socketFd, buffer, bufferSize, 0);
	} while (recvResult < 0 && errno == EINTR);
	if (recvResult < 0)
		throw SocketReceiveException(errno);
	return static_cast<std::int64_t>(recvResult);
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
