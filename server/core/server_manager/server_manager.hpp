#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
    #include <poll.h>
    #include <sys/socket.h>
}

namespace utils {
class Socket;
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
		static std::int32_t pollSockets(std::vector<struct pollfd> &pollFds, std::int32_t timeoutMs);

		std::unique_ptr<utils::Socket> _listenSocket;
		std::uint16_t _port;
};

} // namespace server

#endif //SERVER_MANAGER_HPP
