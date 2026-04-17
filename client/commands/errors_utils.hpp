#ifndef CLIENT_COMMANDS_ERRORS_UTILS_HPP
#define CLIENT_COMMANDS_ERRORS_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif


#include "core/client.hpp"
#include "client/display/printer.hpp"

using client::Client;

namespace client::commands {

void handleNotFound(Client &clientData);

}
#endif // CLIENT_COMMANDS_ERRORS_UTILS_HPP

