#ifndef CLIENT_COMMANDS_COMMAND_DISPATCHER_HPP
#define CLIENT_COMMANDS_COMMAND_DISPATCHER_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void dispatchCommand(Client &clientData, ParsedInput &input);

}

#endif // CLIENT_COMMANDS_COMMAND_DISPATCHER_HPP
