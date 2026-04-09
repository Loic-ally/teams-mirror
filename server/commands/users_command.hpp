#ifndef SERVER_COMMANDS_USERS_COMMAND_HPP
#define SERVER_COMMANDS_USERS_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "server/commands/command_context.hpp"

namespace server::commands {

void handleUsersCommand(CommandContext &context);

} // namespace server::commands

#endif // SERVER_COMMANDS_USERS_COMMAND_HPP
