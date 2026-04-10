#include "server/commands/info_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <cstring>
#include <string>
#include <string_view>

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

void queueUserInfo(CommandContext &context, const myteams::User &user)
{
    myteams::PayloadRplUser payload {};
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), user.getUuid());
    copyPaddedString(payload.user_name, sizeof(payload.user_name), user.getName());
    payload.user_status = user.isLoggedIn() ? 1U : 0U;

    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_USER_INFO, &payload, sizeof(payload)));
}

void queueTeamInfo(CommandContext &context, const myteams::Team &team)
{
    myteams::PayloadRplTeam payload {};
    copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), team.getUuid());
    copyPaddedString(payload.team_name, sizeof(payload.team_name), team.getName());
    copyPaddedString(payload.team_description, sizeof(payload.team_description), team.getDescription());

    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_TEAMS_LIST, &payload, sizeof(payload)));
}

void queueChannelInfo(CommandContext &context, const myteams::Channel &channel)
{
    myteams::PayloadRplChannel payload {};
    copyPaddedString(payload.channel_uuid, sizeof(payload.channel_uuid), channel.getUuid());
    copyPaddedString(payload.channel_name, sizeof(payload.channel_name), channel.getName());
    copyPaddedString(payload.channel_description, sizeof(payload.channel_description), channel.getDescription());

    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CHANNELS_LIST, &payload, sizeof(payload)));
}

void queueThreadInfo(CommandContext &context, const myteams::Thread &thread)
{
    myteams::PayloadRplThread payload {};
    copyPaddedString(payload.thread_uuid, sizeof(payload.thread_uuid), thread.getUuid());
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), thread.getAuthorUuid());
    payload.thread_timestamp = static_cast<std::uint64_t>(thread.getCreatedAt());
    copyPaddedString(payload.thread_title, sizeof(payload.thread_title), thread.getTitle());
    copyPaddedString(payload.thread_body, sizeof(payload.thread_body), thread.getBody());

    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_THREADS_LIST, &payload, sizeof(payload)));
}

} // namespace

void handleInfoCommand(CommandContext &context)
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
        queueUserInfo(context, *authenticatedUser);
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
        queueTeamInfo(context, *team);
        return;
    }

    myteams::Channel *channel = findChannelByUuid(*team, channelUuid);
    if (channel == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    if (threadUuid.empty()) {
        queueChannelInfo(context, *channel);
        return;
    }

    myteams::Thread *thread = findThreadByUuid(*channel, threadUuid);
    if (thread == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    queueThreadInfo(context, *thread);
}

} // namespace server::commands
