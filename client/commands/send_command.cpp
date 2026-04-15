#include "send_command.hpp"
#include "commands/packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"


#include <iostream>
#include <string>

namespace client::commands {

void handleSend(Client &clientData, ParsedInput &input)
{
    (void)clientData;

    std::string uuid;
    std::string message;

    input >> uuid;
    input >> message;

    if (input.hasRemainingArgs() || input.fail()) {
        std::cout << "Usage: send \"user_uuid\" \"message_body\" :" << std::endl;
        return;
    }

    if (uuid.empty() || message.empty()) {
        std::cout << "Arguments cannot be empty." << std::endl;
        return;
    }

    myteams::PayloadReqSendMsg payload{};
    copyPaddedString(payload.message_body, message);
    copyPaddedString(payload.target_uuid, uuid);
    sendPacket(*clientData.socket, buildPacket(myteams::CMD_SEND, payload));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        return;
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

}

}
