#ifndef SENDER_HPP
#define SENDER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>

namespace server {

class ClientManager;

namespace network {

class Sender {
	public:
		static bool flushClientData(ClientManager &clientManager, std::int32_t clientFd);

	private:
		static std::int64_t sendBytes(std::int32_t socketFd, const char *buffer, std::size_t bufferSize);
};

} // namespace network

} // namespace server

#endif // SENDER_HPP
