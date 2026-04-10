#ifndef SERVER_COMMANDS_COMMAND_CONTEXT_HPP
#define SERVER_COMMANDS_COMMAND_CONTEXT_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "server/models/team/team.hpp"
#include "server/models/user/user.hpp"

namespace server {
class ClientManager;
}

namespace utils {
class Socket;
}

namespace server::commands {

using ClientSockets = std::unordered_map<std::int32_t, std::unique_ptr<utils::Socket>>;
using AuthenticatedUserByFd = std::unordered_map<std::int32_t, std::string>;

struct CommandContext {
    ClientManager &clientManager;
    std::int32_t clientFd;
    const char *payloadData;
    std::uint16_t payloadSize;
    std::vector<myteams::User> &users;
    std::vector<myteams::Team> &teams;
    const ClientSockets &clientSockets;
    AuthenticatedUserByFd &authenticatedUsersByFd;
};

} // namespace server::commands

#endif // SERVER_COMMANDS_COMMAND_CONTEXT_HPP

