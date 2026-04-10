#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace server {

class ClientManager {
	public:
		void addClient(std::int32_t socketFd);
		void removeClient(std::int32_t socketFd) noexcept;

		bool hasClient(std::int32_t socketFd) const noexcept;
		bool hasPendingWrite(std::int32_t socketFd) const noexcept;

		void appendToIncomingBuffer(std::int32_t socketFd, const char *data, std::size_t size);
		const std::string &getIncomingBuffer(std::int32_t socketFd) const noexcept;
		void clearIncomingBuffer(std::int32_t socketFd) noexcept;
		void consumeIncomingBuffer(std::int32_t socketFd, std::size_t consumedBytes) noexcept;

		void queueDataToSend(std::int32_t socketFd, const char *data, std::size_t size);
		const char *getQueuedData(std::int32_t socketFd) const noexcept;
		std::size_t getQueuedSize(std::int32_t socketFd) const noexcept;
		void consumeQueuedData(std::int32_t socketFd, std::size_t consumedBytes) noexcept;

	private:
		struct ClientState {
			std::string incomingBuffer;
			std::string outgoingBuffer;
		};
		std::unordered_map<std::int32_t, ClientState> _clients;
};

} // namespace server

#endif // CLIENT_MANAGER_HPP
