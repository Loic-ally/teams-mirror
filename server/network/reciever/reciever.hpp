#ifndef RECIEVER_HPP
#define RECIEVER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>

namespace server {

class ClientManager;

namespace network {

class Reciever {
	public:
		static bool readClientData(ClientManager &clientManager, std::int32_t clientFd);

	private:
		static std::int64_t recvBytes(std::int32_t socketFd, char *buffer, std::size_t bufferSize);
};

} // namespace network

} // namespace server

#endif // RECIEVER_HPP
