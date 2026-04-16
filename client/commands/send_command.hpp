#ifndef CLIENT_COMMANDS_SEND_COMMAND_HPP
#define CLIENT_COMMANDS_SEND_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleSend(Client &clientData, ParsedInput &input);

}

#endif // CLIENT_COMMANDS_SEND_COMMAND_HPP
