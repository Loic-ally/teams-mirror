#include "server/commands/list_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace server::commands {

template <typename PayloadType>
static void queuePayloadArray(
    CommandContext &context,
    const myteams::StatusCode status,
    const std::vector<PayloadType> &payloads)
{
    for (const PayloadType &payload : payloads) {
        queuePacket(
            context.clientManager,
            context.clientFd,
            buildPacket(static_cast<std::uint16_t>(status), payload));
    }
    queueStatus(context, myteams::RPL_OK);
}

static void queueTeamsList(CommandContext &context)
{
    std::vector<myteams::PayloadRplTeam> payloads;
    payloads.reserve(context.teams.size());

    for (const myteams::Team &team : context.teams) {
        myteams::PayloadRplTeam payload {};
        copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), team.getUuid());
        copyPaddedString(payload.team_name, sizeof(payload.team_name), team.getName());
        copyPaddedString(payload.team_description, sizeof(payload.team_description), team.getDescription());
        payloads.push_back(payload);
    }

    queuePayloadArray(context, myteams::RPL_TEAMS_LIST, payloads);
}

static void queueChannelsList(CommandContext &context, const myteams::Team &team)
{
    std::vector<myteams::PayloadRplChannel> payloads;
    payloads.reserve(team.getChannels().size());

    for (const myteams::Channel &channel : team.getChannels()) {
        myteams::PayloadRplChannel payload {};
        copyPaddedString(payload.channel_uuid, sizeof(payload.channel_uuid), channel.getUuid());
        copyPaddedString(payload.channel_name, sizeof(payload.channel_name), channel.getName());
        copyPaddedString(payload.channel_description, sizeof(payload.channel_description), channel.getDescription());
        payloads.push_back(payload);
    }

    queuePayloadArray(context, myteams::RPL_CHANNELS_LIST, payloads);
}

static void queueThreadsList(CommandContext &context, const myteams::Channel &channel)
{
    std::vector<myteams::PayloadRplThread> payloads;
    payloads.reserve(channel.getThreads().size());

    for (const myteams::Thread &thread : channel.getThreads()) {
        myteams::PayloadRplThread payload {};
        copyPaddedString(payload.thread_uuid, sizeof(payload.thread_uuid), thread.getUuid());
        copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), thread.getAuthorUuid());
        payload.thread_timestamp = static_cast<std::uint64_t>(thread.getCreatedAt());
        copyPaddedString(payload.thread_title, sizeof(payload.thread_title), thread.getTitle());
        copyPaddedString(payload.thread_body, sizeof(payload.thread_body), thread.getBody());
        payloads.push_back(payload);
    }

    queuePayloadArray(context, myteams::RPL_THREADS_LIST, payloads);
}

static void queueRepliesList(CommandContext &context, const myteams::Thread &thread)
{
    std::vector<myteams::PayloadRplReply> payloads;
    payloads.reserve(thread.getReplies().size());

    for (const myteams::Message &reply : thread.getReplies()) {
        myteams::PayloadRplReply payload {};
        copyPaddedString(payload.thread_uuid, sizeof(payload.thread_uuid), thread.getUuid());
        copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), reply.getAuthorUuid());
        payload.reply_timestamp = static_cast<std::uint64_t>(reply.getCreatedAt());
        copyPaddedString(payload.reply_body, sizeof(payload.reply_body), reply.getBody());
        payloads.push_back(payload);
    }

    queuePayloadArray(context, myteams::RPL_REPLIES_LIST, payloads);
}

void handleListCommand(CommandContext &context)
{
    if (context.payloadSize != 0) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    const auto authenticatedUser = getAuthenticatedUser(context);
    if (!authenticatedUser.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    const myteams::User &resolvedUser = authenticatedUser->get();

    const std::string teamUuid = context.clientManager.getContextTeamUuid(context.clientFd);
    const std::string channelUuid = context.clientManager.getContextChannelUuid(context.clientFd);
    const std::string threadUuid = context.clientManager.getContextThreadUuid(context.clientFd);

    if (!isContextCombinationValid(teamUuid, channelUuid, threadUuid)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    if (teamUuid.empty()) {
        queueTeamsList(context);
        return;
    }

    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Team &resolvedTeam = team->get();
    if (!resolvedTeam.isUserSubscribed(resolvedUser.getUuid())) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    if (channelUuid.empty()) {
        queueChannelsList(context, resolvedTeam);
        return;
    }

    const auto channel = findChannelByUuid(resolvedTeam, channelUuid);
    if (!channel.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Channel &resolvedChannel = channel->get();

    if (threadUuid.empty()) {
        queueThreadsList(context, resolvedChannel);
        return;
    }

    const auto thread = findThreadByUuid(resolvedChannel, threadUuid);
    if (!thread.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    queueRepliesList(context, thread->get());
}

} // namespace server::commands
