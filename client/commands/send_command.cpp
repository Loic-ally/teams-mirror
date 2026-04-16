#include "send_command.hpp"
#include "common/protocol.hpp"
#include "display/printer.hpp"
#include "packet_utils.hpp"

#include <iostream>
#include <string>

namespace client::commands {

void handleSend(Client &clientData, ParsedInput &input)
{
    const std::string targetUuid = input.getArg<std::string>();
    const std::string messageBody = input.getArg<std::string>();

    if (input.fail() || input.hasRemainingArgs()) {
        std::cout << "Usage: /send \"user_uuid\" \"message_body\"" << std::endl;
        return;
    }
    if (!isUuidFormatValid(targetUuid)) {
        std::cout << "Invalid user UUID format." << std::endl;
        return;
    }
    if (messageBody.empty()) {
        std::cout << "Message body cannot be empty." << std::endl;
        return;
    }

    myteams::PayloadReqSendMsg payload{};
    copyPaddedString(payload.target_uuid, sizeof(payload.target_uuid), targetUuid);
    copyPaddedString(payload.message_body, sizeof(payload.message_body), messageBody);
    sendPacket(*clientData.socket, buildPacket(myteams::CMD_SEND, payload));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        return;
    }
    if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (responseHeader.code == myteams::ERR_NOT_FOUND) {
        Printer::errorUnknownUser(targetUuid);
        return;
    }
    if (responseHeader.code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid /send request." << std::endl;
        return;
    }
    std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
}

}
