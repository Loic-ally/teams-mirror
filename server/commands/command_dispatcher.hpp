#ifndef SERVER_COMMANDS_COMMAND_DISPATCHER_HPP
#define SERVER_COMMANDS_COMMAND_DISPATCHER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <vector>

#include "server/commands/command_context.hpp"
#include "server/models/team/team.hpp"
#include "server/models/user/user.hpp"

namespace server::commands {

void processClientIncomingPackets(
    ClientManager &clientManager,
    std::int32_t clientFd,
    std::vector<myteams::User> &users,
    std::vector<myteams::Team> &teams,
    std::vector<myteams::Message> &messages,
    const ClientSockets &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd,
    AuthenticatedUserByUUID &authenticatedUsersByUUID
);

void handleClientDisconnection(
    ClientManager &clientManager,
    std::int32_t clientFd,
    std::vector<myteams::User> &users,
    AuthenticatedUserByFd &authenticatedUsersByFd,
    AuthenticatedUserByUUID &authenticatedUsersByUUID
);

} // namespace server::commands

#endif // SERVER_COMMANDS_COMMAND_DISPATCHER_HPP

