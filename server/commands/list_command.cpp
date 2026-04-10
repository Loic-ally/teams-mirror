#include "server/commands/list_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace server::commands {
namespace {

myteams::User *getAuthenticatedUser(CommandContext &context)
{
    const auto authenticatedUserIt = context.authenticatedUsersByFd.find(context.clientFd);
    if (authenticatedUserIt == context.authenticatedUsersByFd.end()) {
        return nullptr;
    }
    return findUserByUuid(context.users, authenticatedUserIt->second);
}

myteams::Channel *findChannelByUuid(myteams::Team &team, const std::string_view channelUuid)
{
    for (myteams::Channel &channel : team.getChannels()) {
        if (channel.getUuid() == channelUuid) {
            return &channel;
        }
    }
    return nullptr;
}

myteams::Thread *findThreadByUuid(myteams::Channel &channel, const std::string_view threadUuid)
{
    for (myteams::Thread &thread : channel.getThreads()) {
        if (thread.getUuid() == threadUuid) {
            return &thread;
        }
    }
    return nullptr;
}

template <typename PayloadType>
bool queuePayloadArray(
    CommandContext &context,
    const myteams::StatusCode status,
    const std::vector<PayloadType> &payloads)
{
    if (payloads.empty()) {
        queuePacket(
            context.clientManager,
            context.clientFd,
            buildPacket(static_cast<std::uint16_t>(status)));
        return true;
    }

    const std::size_t payloadSize = payloads.size() * sizeof(PayloadType);
    if (payloadSize > std::numeric_limits<std::uint16_t>::max()) {
        queueStatus(context, myteams::ERR_SERVER_INTERNAL);
        return false;
    }

    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(
            static_cast<std::uint16_t>(status),
            payloads.data(),
            static_cast<std::uint16_t>(payloadSize)));
    return true;
}

void queueTeamsList(CommandContext &context)
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

    (void)queuePayloadArray(context, myteams::RPL_TEAMS_LIST, payloads);
}

void queueChannelsList(CommandContext &context, const myteams::Team &team)
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

    (void)queuePayloadArray(context, myteams::RPL_CHANNELS_LIST, payloads);
}

void queueThreadsList(CommandContext &context, const myteams::Channel &channel)
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

    (void)queuePayloadArray(context, myteams::RPL_THREADS_LIST, payloads);
}

void queueRepliesList(CommandContext &context, const myteams::Thread &thread)
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

    (void)queuePayloadArray(context, myteams::RPL_REPLIES_LIST, payloads);
}

bool isContextCombinationValid(
    const std::string &teamUuid,
    const std::string &channelUuid,
    const std::string &threadUuid)
{
    if (teamUuid.empty() && (!channelUuid.empty() || !threadUuid.empty())) {
        return false;
    }
    if (channelUuid.empty() && !threadUuid.empty()) {
        return false;
    }
    return true;
}

} // namespace

void handleListCommand(CommandContext &context)
{
    if (context.payloadSize != 0) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    myteams::User *authenticatedUser = getAuthenticatedUser(context);
    if (authenticatedUser == nullptr) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

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

    myteams::Team *team = findTeamByUuid(context.teams, teamUuid);
    if (team == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    if (!team->isUserSubscribed(authenticatedUser->getUuid())) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    if (channelUuid.empty()) {
        queueChannelsList(context, *team);
        return;
    }

    myteams::Channel *channel = findChannelByUuid(*team, channelUuid);
    if (channel == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    if (threadUuid.empty()) {
        queueThreadsList(context, *channel);
        return;
    }

    myteams::Thread *thread = findThreadByUuid(*channel, threadUuid);
    if (thread == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    queueRepliesList(context, *thread);
}

} // namespace server::commands
