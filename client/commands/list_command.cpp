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

template <typename PayloadType>
static bool isPayloadArrayWellFormed(const std::string &payload)
{
    return (payload.size() % sizeof(PayloadType)) == 0;
}

static void printTeamsList(const std::string &payload)
{
    if (!isPayloadArrayWellFormed<myteams::PayloadRplTeam>(payload)) {
        std::cout << "Malformed teams list payload received from server." << std::endl;
        return;
    }

    const std::size_t teamCount = payload.size() / sizeof(myteams::PayloadRplTeam);
    for (std::size_t index = 0; index < teamCount; ++index) {
        myteams::PayloadRplTeam teamPayload {};
        std::memcpy(
            &teamPayload,
            payload.data() + (index * sizeof(teamPayload)),
            sizeof(teamPayload));
        Printer::printTeams(
            teamPayload.team_uuid,
            teamPayload.team_name,
            teamPayload.team_description);
    }
}

static void printChannelsList(const std::string &payload)
{
    if (!isPayloadArrayWellFormed<myteams::PayloadRplChannel>(payload)) {
        std::cout << "Malformed channels list payload received from server." << std::endl;
        return;
    }

    const std::size_t channelCount = payload.size() / sizeof(myteams::PayloadRplChannel);
    for (std::size_t index = 0; index < channelCount; ++index) {
        myteams::PayloadRplChannel channelPayload {};
        std::memcpy(
            &channelPayload,
            payload.data() + (index * sizeof(channelPayload)),
            sizeof(channelPayload));
        Printer::printTeamChannels(
            channelPayload.channel_uuid,
            channelPayload.channel_name,
            channelPayload.channel_description);
    }
}

static void printThreadsList(const std::string &payload)
{
    if (!isPayloadArrayWellFormed<myteams::PayloadRplThread>(payload)) {
        std::cout << "Malformed threads list payload received from server." << std::endl;
        return;
    }

    const std::size_t threadCount = payload.size() / sizeof(myteams::PayloadRplThread);
    for (std::size_t index = 0; index < threadCount; ++index) {
        myteams::PayloadRplThread threadPayload {};
        std::memcpy(
            &threadPayload,
            payload.data() + (index * sizeof(threadPayload)),
            sizeof(threadPayload));
        Printer::printChannelThreads(
            threadPayload.thread_uuid,
            threadPayload.user_uuid,
            static_cast<std::time_t>(threadPayload.thread_timestamp),
            threadPayload.thread_title,
            threadPayload.thread_body);
    }
}

static void printRepliesList(const std::string &payload)
{
    if (!isPayloadArrayWellFormed<myteams::PayloadRplReply>(payload)) {
        std::cout << "Malformed replies list payload received from server." << std::endl;
        return;
    }

    const std::size_t replyCount = payload.size() / sizeof(myteams::PayloadRplReply);
    for (std::size_t index = 0; index < replyCount; ++index) {
        myteams::PayloadRplReply replyPayload {};
        std::memcpy(
            &replyPayload,
            payload.data() + (index * sizeof(replyPayload)),
            sizeof(replyPayload));
        Printer::printThreadReplies(
            replyPayload.thread_uuid,
            replyPayload.user_uuid,
            static_cast<std::time_t>(replyPayload.reply_timestamp),
            replyPayload.reply_body);
    }
}

static void handleListError(const std::uint16_t code)
{
    if (code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
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

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_TEAMS_LIST) {
        printTeamsList(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_CHANNELS_LIST) {
        printChannelsList(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_THREADS_LIST) {
        printThreadsList(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_REPLIES_LIST) {
        printRepliesList(responsePayload);
        return;
    }

    handleListError(responseHeader.code);
}

} // namespace client::commands
