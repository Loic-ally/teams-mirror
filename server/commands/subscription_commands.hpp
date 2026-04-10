#ifndef SERVER_COMMANDS_SUBSCRIPTION_COMMANDS_HPP
#define SERVER_COMMANDS_SUBSCRIPTION_COMMANDS_HPP

#ifdef _WIN32
#pragma once
#endif

#include "server/commands/command_context.hpp"

namespace server::commands {

void handleSubscribeCommand(CommandContext &context);
void handleUnsubscribeCommand(CommandContext &context);
void handleSubscribedListCommand(CommandContext &context);

} // namespace server::commands

#endif // SERVER_COMMANDS_SUBSCRIPTION_COMMANDS_HPP
