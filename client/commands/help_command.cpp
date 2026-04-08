#include "help_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"

#include <iostream>

namespace client::commands {

void handleHelp(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        std::cout << "Usage: /help" << std::endl;
        return;
    }
    const std::string packet = buildPacket(myteams::CMD_INFO);
    sendPacket(*clientData.socket, packet);
    std::cout
        << "/help: display this help\n"
        << "/login \"user_name\": log in with a username\n"
        << "/logout: log out from the current session"
        << std::endl;
}

}
