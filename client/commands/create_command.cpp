#include "create_command.hpp"
#include "commands/errors_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "packet_utils.hpp"
#include "parser/parser.hpp"

#include <ctime>
#include <cstring>
#include <iostream>
#include <string>

namespace client::commands {

static CreateTarget inferCreateTargetFromContext(const Client &clientData)
{
    if (clientData.contextTeamUuid.empty()) {
        return CreateTarget::Team;
    }
    if (clientData.contextChannelUuid.empty()) {
        return CreateTarget::Channel;
    }
    if (clientData.contextThreadUuid.empty()) {
        return CreateTarget::Thread;
    }
    return CreateTarget::Reply;
}

static void printUnexpectedPayload(const std::string_view message)
{
    std::cout << message << std::endl;
}

static void handleCreatedReply(const CreateTarget target, const std::string &payload)
{
    if (target == CreateTarget::Team) {
        if (payload.size() != sizeof(myteams::PayloadRplTeam)) {
            printUnexpectedPayload("Malformed created team payload received from server.");
            return;
        }
        myteams::PayloadRplTeam createdPayload {};
        std::memcpy(&createdPayload, payload.data(), sizeof(createdPayload));
        Printer::printTeamCreated(
            createdPayload.team_uuid,
            createdPayload.team_name,
            createdPayload.team_description);
        return;
    }
    if (target == CreateTarget::Channel) {
        if (payload.size() != sizeof(myteams::PayloadRplChannel)) {
            printUnexpectedPayload("Malformed created channel payload received from server.");
            return;
        }
        myteams::PayloadRplChannel createdPayload {};
        std::memcpy(&createdPayload, payload.data(), sizeof(createdPayload));
        Printer::printChannelCreated(
            createdPayload.channel_uuid,
            createdPayload.channel_name,
            createdPayload.channel_description);
        return;
    }
    if (target == CreateTarget::Thread) {
        if (payload.size() != sizeof(myteams::PayloadRplThread)) {
            printUnexpectedPayload("Malformed created thread payload received from server.");
            return;
        }
        myteams::PayloadRplThread createdPayload {};
        std::memcpy(&createdPayload, payload.data(), sizeof(createdPayload));
        Printer::printThreadCreated(
            createdPayload.thread_uuid,
            createdPayload.user_uuid,
            static_cast<std::time_t>(createdPayload.thread_timestamp),
            createdPayload.thread_title,
            createdPayload.thread_body);
        return;
    }

    if (payload.size() != sizeof(myteams::PayloadRplReply)) {
        printUnexpectedPayload("Malformed created reply payload received from server.");
        return;
    }
    myteams::PayloadRplReply createdPayload {};
    std::memcpy(&createdPayload, payload.data(), sizeof(createdPayload));
    Printer::printReplyCreated(
        createdPayload.thread_uuid,
        createdPayload.user_uuid,
        static_cast<std::time_t>(createdPayload.reply_timestamp),
        createdPayload.reply_body);
}

static void handleCreateError(const std::uint16_t code, Client &clientData)
{
    if (code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (code == myteams::ERR_NOT_FOUND) {
        return handleNotFound(clientData);
    }
    if (code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid create request for current context." << std::endl;
        return;
    }
    std::cout << "Server returned unexpected status: " << code << std::endl;
}

void handleCreate(Client &clientData, ParsedInput &input)
{
    if (!clientData.connected) {
        std::cout << "Unauthorized" << std::endl;
        return;
    }

    const CreateTarget target = inferCreateTargetFromContext(clientData);

    if (target == CreateTarget::Reply) {
        const std::string replyBody = input.getArg<std::string>();
        if (input.fail() || input.hasRemainingArgs() || replyBody.empty()) {
            std::cout << "Usage: /create \"reply_body\"" << std::endl;
            return;
        }

        myteams::PayloadReqCreateReply payload {};
        copyPaddedString(payload.reply_body, sizeof(payload.reply_body), replyBody);
        sendPacket(*clientData.socket, buildPacket(myteams::CMD_CREATE, payload));
    } else {
        const std::string firstArg = input.getArg<std::string>();
        const std::string secondArg = input.getArg<std::string>();
        if (input.fail() || input.hasRemainingArgs() || firstArg.empty() || secondArg.empty()) {
            std::cout << "Usage: /create \"name\" \"description\" or /create \"reply_body\"" << std::endl;
            return;
        }

        if (target == CreateTarget::Team) {
            myteams::PayloadReqCreateTeam payload {};
            copyPaddedString(payload.team_name, sizeof(payload.team_name), firstArg);
            copyPaddedString(payload.team_description, sizeof(payload.team_description), secondArg);
            sendPacket(*clientData.socket, buildPacket(myteams::CMD_CREATE, payload));
        } else if (target == CreateTarget::Channel) {
            myteams::PayloadReqCreateChannel payload {};
            copyPaddedString(payload.channel_name, sizeof(payload.channel_name), firstArg);
            copyPaddedString(payload.channel_description, sizeof(payload.channel_description), secondArg);
            sendPacket(*clientData.socket, buildPacket(myteams::CMD_CREATE, payload));
        } else {
            myteams::PayloadReqCreateThread payload {};
            copyPaddedString(payload.thread_title, sizeof(payload.thread_title), firstArg);
            copyPaddedString(payload.thread_body, sizeof(payload.thread_body), secondArg);
            sendPacket(*clientData.socket, buildPacket(myteams::CMD_CREATE, payload));
        }
    }

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_CREATED) {
        handleCreatedReply(target, responsePayload);
        return;
    }
    handleCreateError(responseHeader.code, clientData);
}

} // namespace client::commands
