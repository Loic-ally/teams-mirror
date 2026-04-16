#ifndef SERVER_COMMANDS_SEND_COMMAND_HPP
#define SERVER_COMMANDS_SEND_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <vector>

#include "server/commands/command_context.hpp"

namespace server::commands {

void handleSendCommand(CommandContext &context);

} // namespace server::commands

#endif // SERVER_COMMANDS_SEND_COMMAND_HPP
