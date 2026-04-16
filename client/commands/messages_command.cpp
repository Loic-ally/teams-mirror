#include "messages_command.hpp"
#include "common/protocol.hpp"
#include "display/printer.hpp"
#include "packet_utils.hpp"

#include <ctime>
#include <cstring>
#include <iostream>
#include <string>

namespace client::commands {

static void printMessageEntry(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplMessage)) {
        std::cout << "Malformed messages payload received from server." << std::endl;
        return;
    }

    myteams::PayloadRplMessage messagePayload {};
    std::memcpy(&messagePayload, payload.data(), sizeof(messagePayload));
    Printer::printPrivateMessages(
        messagePayload.sender_uuid,
        static_cast<std::time_t>(messagePayload.message_timestamp),
        messagePayload.message_body);
}

void handleMessages(Client &clientData, ParsedInput &input)
{
    const std::string targetUuid = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        std::cout << "Usage: /messages \"user_uuid\"" << std::endl;
        return;
    }
    if (!isUuidFormatValid(targetUuid)) {
        std::cout << "Invalid user UUID format." << std::endl;
        return;
    }

    myteams::PayloadReqTargetUser payload{};
    copyPaddedString(payload.target_uuid, sizeof(payload.target_uuid), targetUuid);
    sendPacket(*clientData.socket, buildPacket(myteams::CMD_MESSAGES, payload));

    while (true) {
        myteams::PacketHeader responseHeader {};
        std::string responsePayload;
        readServerReply(*clientData.socket, responseHeader, responsePayload);

        if (responseHeader.code == myteams::RPL_OK) {
            return;
        }
        if (responseHeader.code == myteams::RPL_MESSAGES_LIST) {
            printMessageEntry(responsePayload);
            continue;
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
            std::cout << "Invalid /messages request." << std::endl;
            return;
        }
        std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
        return;
    }
}

}
