#include "list_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

namespace client::commands {

static void printTeamsList(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplTeam)) {
        std::cout << "Malformed teams list payload received from server." << std::endl;
        return;
    }

    myteams::PayloadRplTeam teamPayload {};
    std::memcpy(&teamPayload, payload.data(), sizeof(teamPayload));
    Printer::printTeams(
        teamPayload.team_uuid,
        teamPayload.team_name,
        teamPayload.team_description);
}

static void printChannelsList(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplChannel)) {
        std::cout << "Malformed channels list payload received from server." << std::endl;
        return;
    }

    myteams::PayloadRplChannel channelPayload {};
    std::memcpy(&channelPayload, payload.data(), sizeof(channelPayload));
    Printer::printTeamChannels(
        channelPayload.channel_uuid,
        channelPayload.channel_name,
        channelPayload.channel_description);
}

static void printThreadsList(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplThread)) {
        std::cout << "Malformed threads list payload received from server." << std::endl;
        return;
    }

    myteams::PayloadRplThread threadPayload {};
    std::memcpy(&threadPayload, payload.data(), sizeof(threadPayload));
    Printer::printChannelThreads(
        threadPayload.thread_uuid,
        threadPayload.user_uuid,
        static_cast<std::time_t>(threadPayload.thread_timestamp),
        threadPayload.thread_title,
        threadPayload.thread_body);
}

static void printRepliesList(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplReply)) {
        std::cout << "Malformed replies list payload received from server." << std::endl;
        return;
    }

    myteams::PayloadRplReply replyPayload {};
    std::memcpy(&replyPayload, payload.data(), sizeof(replyPayload));
    Printer::printThreadReplies(
        replyPayload.thread_uuid,
        replyPayload.user_uuid,
        static_cast<std::time_t>(replyPayload.reply_timestamp),
        replyPayload.reply_body);
}

static void handleListError(const std::uint16_t code)
{
    if (code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (code == myteams::ERR_FORBIDDEN) {
        std::cout << "You are not allowed to list this context." << std::endl;
        return;
    }
    if (code == myteams::ERR_NOT_FOUND) {
        std::cout << "Requested context entity does not exist." << std::endl;
        return;
    }
    if (code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid context for /list." << std::endl;
        return;
    }

    std::cout << "Server returned unexpected status: " << code << std::endl;
}

void handleList(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        std::cout << "Usage: /list" << std::endl;
        return;
    }

    sendPacket(*clientData.socket, buildPacket(myteams::CMD_LIST));

    std::uint16_t expectedListCode = 0;

    while (true) {
        myteams::PacketHeader responseHeader {};
        std::string responsePayload;
        readServerReply(*clientData.socket, responseHeader, responsePayload);

        if (responseHeader.code == myteams::RPL_OK) {
            return;
        }
        if (responseHeader.code == myteams::ERR_UNAUTHORIZED
            || responseHeader.code == myteams::ERR_FORBIDDEN
            || responseHeader.code == myteams::ERR_NOT_FOUND
            || responseHeader.code == myteams::ERR_BAD_REQUEST) {
            handleListError(responseHeader.code);
            return;
        }

        if (expectedListCode == 0) {
            expectedListCode = responseHeader.code;
        }
        if (responseHeader.code != expectedListCode) {
            std::cout << "Unexpected response sequence for /list." << std::endl;
            return;
        }

        if (responseHeader.code == myteams::RPL_TEAMS_LIST) {
            printTeamsList(responsePayload);
            continue;
        }
        if (responseHeader.code == myteams::RPL_CHANNELS_LIST) {
            printChannelsList(responsePayload);
            continue;
        }
        if (responseHeader.code == myteams::RPL_THREADS_LIST) {
            printThreadsList(responsePayload);
            continue;
        }
        if (responseHeader.code == myteams::RPL_REPLIES_LIST) {
            printRepliesList(responsePayload);
            continue;
        }

        handleListError(responseHeader.code);
        return;
    }
}

} // namespace client::commands
