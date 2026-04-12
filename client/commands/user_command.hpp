#ifndef CLIENT_COMMANDS_USER_COMMAND_HPP
#define CLIENT_COMMANDS_USER_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif
namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleUser(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif //CLIENT_COMMANDS_USER_COMMAND_HPP
