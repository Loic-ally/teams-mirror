#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "server/core/client_manager/client.hpp"

namespace server {

class ClientManager {
	public:
		void addClient(std::int32_t socketFd);
		void removeClient(std::int32_t socketFd) noexcept;

		bool hasClient(std::int32_t socketFd) const noexcept;
		bool hasPendingWrite(std::int32_t socketFd) const noexcept;

		void appendToIncomingBuffer(std::int32_t socketFd, std::string_view data);
		const std::string &getIncomingBuffer(std::int32_t socketFd) const noexcept;
		void clearIncomingBuffer(std::int32_t socketFd) noexcept;
		void consumeIncomingBuffer(std::int32_t socketFd, std::size_t consumedBytes) noexcept;

		void queueDataToSend(std::int32_t socketFd, std::string_view data);
		std::string_view getQueuedData(std::int32_t socketFd) const noexcept;
		std::size_t getQueuedSize(std::int32_t socketFd) const noexcept;
		void consumeQueuedData(std::int32_t socketFd, std::size_t consumedBytes) noexcept;

		void setContext(
			std::int32_t socketFd,
			std::string_view teamUuid,
			std::string_view channelUuid,
			std::string_view threadUuid) noexcept;

		std::string getContextTeamUuid(std::int32_t socketFd) const noexcept;
		std::string getContextChannelUuid(std::int32_t socketFd) const noexcept;
		std::string getContextThreadUuid(std::int32_t socketFd) const noexcept;

	private:
		std::unordered_map<std::int32_t, Client> _clients;
};

} // namespace server

#endif // CLIENT_MANAGER_HPP
