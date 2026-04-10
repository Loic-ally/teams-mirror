#ifndef CLIENT_COMMANDS_SUBSCRIPTION_COMMANDS_HPP
#define CLIENT_COMMANDS_SUBSCRIPTION_COMMANDS_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleSubscribe(Client &clientData, ParsedInput &input);
void handleUnsubscribe(Client &clientData, ParsedInput &input);
void handleSubscribedList(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif // CLIENT_COMMANDS_SUBSCRIPTION_COMMANDS_HPP
