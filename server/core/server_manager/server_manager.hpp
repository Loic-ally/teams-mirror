#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
    #include <poll.h>
    #include <sys/socket.h>
}

namespace server {

class ServerManager {
	public:
		explicit ServerManager(std::uint16_t port);
		~ServerManager() noexcept;

		ServerManager(const ServerManager &) = delete;
		ServerManager &operator=(const ServerManager &) = delete;
		ServerManager(ServerManager &&other) noexcept;
		ServerManager &operator=(ServerManager &&other) noexcept;

		void initializeTcpListener(std::int32_t backlog = SOMAXCONN);
		void runPollLoop();

		std::int32_t getListenFd() const noexcept;
		std::uint16_t getPort() const noexcept;

	private:
		static void closeSocket(std::int32_t &fd) noexcept;
		static std::int32_t createTcpSocket();
		static void setReuseAddress(std::int32_t socketFd);
		static std::uint32_t toNetworkAddress(std::uint32_t hostAddress);
		static std::uint16_t toNetworkPort(std::uint16_t hostPort);
		static std::int32_t pollSockets(std::vector<struct pollfd> &pollFds, std::int32_t timeoutMs);
		static std::int32_t acceptClient(std::int32_t socketFd);
		void bindSocket(std::int32_t socketFd) const;
		static void listenSocket(std::int32_t socketFd, std::int32_t backlog);

		std::int32_t _listenFd;
		std::uint16_t _port;
};

} // namespace server

#endif //SERVER_MANAGER_HPP
