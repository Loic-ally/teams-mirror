#ifndef CLIENT_CORE_CLIENT_HPP
#define CLIENT_CORE_CLIENT_HPP

#ifdef _WIN32
#pragma once
#endif

#include "Socket.hpp"

#include <memory>
#include <string>

namespace client {

class Client {
    public:
        std::unique_ptr<utils::Socket> socket;
        bool running = true;
        bool connected = false;
        std::string username;
        std::string contextTeamUuid;
        std::string contextChannelUuid;
        std::string contextThreadUuid;
};

} // namespace client

#endif // CLIENT_CORE_CLIENT_HPP
