#ifndef CLIENT_COMMANDS_MESSAGES_COMMAND_HPP
#define CLIENT_COMMANDS_MESSAGES_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleMessages(Client &clientData, ParsedInput &input);

}

#endif // CLIENT_COMMANDS_MESSAGES_COMMAND_HPP
