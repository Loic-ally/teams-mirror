#ifndef SERVER_COMMANDS_MESSAGES_COMMAND_HPP
#define SERVER_COMMANDS_MESSAGES_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <vector>

#include "server/commands/command_context.hpp"

namespace server::commands {

void handleMessagesCommand(CommandContext &context);

} // namespace server::commands

#endif // SERVER_COMMANDS_MESSAGES_COMMAND_HPP
