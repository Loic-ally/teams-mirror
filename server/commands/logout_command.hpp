#ifndef SERVER_COMMANDS_LOGOUT_COMMAND_HPP
#define SERVER_COMMANDS_LOGOUT_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <vector>

#include "server/commands/command_context.hpp"
#include "server/models/user/user.hpp"

namespace server::commands {

void handleLogoutCommand(CommandContext &context);

void handleClientDisconnection(
    ClientManager &clientManager,
    std::int32_t clientFd,
    std::vector<myteams::User> &users,
    const ClientSockets &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd);

} // namespace server::commands

#endif // SERVER_COMMANDS_LOGOUT_COMMAND_HPP

