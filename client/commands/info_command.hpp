#ifndef CLIENT_COMMANDS_INFO_COMMAND_HPP
#define CLIENT_COMMANDS_INFO_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif

#include "client/core/client.hpp"
#include "client/parser/parser.hpp"

namespace client::commands {

void handleInfo(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif // CLIENT_COMMANDS_INFO_COMMAND_HPP
