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
        << "/logout: log out from the current session\n"
        << "/use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]: set context\n"
        << "/create \"name\" \"description\" or /create \"body\": create by current context\n"
        << "/subscribe \"team_uuid\": subscribe to a team\n"
        << "/unsubscribe \"team_uuid\": unsubscribe from a team\n"
        << "/subscribed [\"team_uuid\"]: list subscribed teams or users"
        << std::endl;
}

}
