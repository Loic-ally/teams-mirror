#include "logout_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cstdint>
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
        Printer::errorUnauthorized();
        return;
    }

    const std::string previousUsername = clientData.username;
    const std::string packet = buildPacket(myteams::CMD_LOGOUT);
    sendPacket(*clientData.socket, packet);

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        clientData.connected = false;
        clientData.username.clear();
        clientData.contextTeamUuid.clear();
        clientData.contextChannelUuid.clear();
        clientData.contextThreadUuid.clear();
        return;
    }
    if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (responseHeader.code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid logout request." << std::endl;
        return;
    }
    std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
}

}
