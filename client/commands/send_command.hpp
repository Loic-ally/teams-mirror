#ifndef CLIENT_COMMANDS_SEND_COMMAND_HPP
#define CLIENT_COMMANDS_SEND_COMMAND_HPP

#ifdef _WIN32
#pragma once
#endif
namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleSend(Client &clientData, ParsedInput &input);

} // namespace client::commands

#endif //CLIENT_COMMANDS_SEND_COMMAND_HPP
