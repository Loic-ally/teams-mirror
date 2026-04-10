#ifndef RECIEVER_HPP
#define RECIEVER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <string>

namespace server {

class ClientManager;

namespace network {

class Reciever {
	public:
		static bool readClientData(ClientManager &clientManager, std::int32_t clientFd);

	private:
		static std::string recvBytes(std::int32_t socketFd, std::size_t bufferSize);
};

} // namespace network

} // namespace server

#endif // RECIEVER_HPP
