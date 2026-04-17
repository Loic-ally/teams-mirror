#include "login_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace client::commands {

void handleLogin(Client &clientData, ParsedInput &input)
{
    if (clientData.connected) {
        std::cout << "You are already logged in as \"" << clientData.username
                  << "\". Use /logout first." << std::endl;
        return;
    }

    const std::string username = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        std::cout << "Usage: /login \"user_name\"" << std::endl;
        return;
    }
    if (username.empty()) {
        std::cout << "Username cannot be empty." << std::endl;
        return;
    }

    myteams::PayloadReqLogin payload {};
    copyPaddedString(payload.user_name, sizeof(payload.user_name), username);
    const std::string packet = buildPacket(myteams::CMD_LOGIN, payload);
    sendPacket(*clientData.socket, packet);

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        clientData.connected = true;
        clientData.username = username;
        return;
    }
    if (responseHeader.code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid login request." << std::endl;
        return;
    }
    std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
}

}
