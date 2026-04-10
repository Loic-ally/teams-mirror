#ifndef CLIENT_COMMANDS_LIST_COMMAND_HPP
#define CLIENT_COMMANDS_LIST_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleList(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif // CLIENT_COMMANDS_LIST_COMMAND_HPP
