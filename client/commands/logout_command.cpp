#include "logout_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <iostream>
#include <string>

namespace client::commands {

void handleLogout(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        std::cout << "Usage: /logout" << std::endl;
        return;
    }
    if (!clientData.connected) {
        (void)Printer::errorUnauthorized();
        return;
    }

    const std::string previousUsername = clientData.username;
    const std::string packet = buildPacket(myteams::CMD_LOGOUT);
    sendPacket(*clientData.socket, packet);

    clientData.connected = false;
    clientData.username.clear();
    (void)Printer::eventLoggedOut("", previousUsername);
}

}
