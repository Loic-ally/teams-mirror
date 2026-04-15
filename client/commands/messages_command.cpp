#include "messages_command.hpp"
#include "commands/packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"


#include <iostream>
#include <string>

namespace client::commands {

void logPrivateMessage(std::string payload) {
    if (payload.size() != sizeof(myteams::PayloadRplMessage)) {
        throw std::runtime_error("invalid RPL_MESSAGES_LIST payload size");
    }
    myteams::PayloadRplMessage message {};
    std::memcpy(
        &message,
        payload.data(),
        sizeof(message));
    Printer::printPrivateMessages(message.sender_uuid,
        message.message_timestamp,
        message.message_body);
}

void handleMessages(Client &clientData, ParsedInput &input)
{
    (void)clientData;

    std::string uuid;

    input >> uuid;

    if (input.hasRemainingArgs() || input.fail()) {
        std::cout << "Usage: messages \"user_uuid\" :" << std::endl;
        return;
    }

    if (uuid.empty()) {
        std::cout << "Arguments cannot be empty." << std::endl;
        return;
    }

    myteams::PayloadReqTargetUser payload{};
    copyPaddedString(payload.target_uuid, uuid);
    sendPacket(*clientData.socket, buildPacket(myteams::CMD_MESSAGES, payload));

    while (true) {
        myteams::PacketHeader responseHeader {};
        std::string responsePayload;
        readServerReply(*clientData.socket, responseHeader, responsePayload);

        if (responseHeader.code == myteams::RPL_OK) {
            return;
        }
        if (responseHeader.code == myteams::RPL_MESSAGES_LIST) {
            logPrivateMessage(responsePayload);
            continue;
        }
        if (responseHeader.code == myteams::ERR_ALREADY_EXIST) {
            Printer::errorAlreadyExist();
            return;
        }
        if (responseHeader.code == myteams::ERR_BAD_REQUEST) {
            std::cout << "Invalid send request." << std::endl;
            return;
        }
        std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
        return;
    }
}

}
