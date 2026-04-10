#ifndef CLIENT_COMMANDS_CREATE_COMMAND_HPP
#define CLIENT_COMMANDS_CREATE_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleCreate(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif // CLIENT_COMMANDS_CREATE_COMMAND_HPP
