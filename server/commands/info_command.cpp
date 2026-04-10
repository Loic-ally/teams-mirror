#include "server/commands/info_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <string>

namespace server::commands {
static void queueUserInfo(CommandContext &context, const myteams::User &user)
{
    myteams::PayloadRplUser payload {};
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), user.getUuid());
    copyPaddedString(payload.user_name, sizeof(payload.user_name), user.getName());
    payload.user_status = user.isLoggedIn() ? 1U : 0U;
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_USER_INFO, payload));
}

static void queueTeamInfo(CommandContext &context, const myteams::Team &team)
{
    myteams::PayloadRplTeam payload {};
    copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), team.getUuid());
    copyPaddedString(payload.team_name, sizeof(payload.team_name), team.getName());
    copyPaddedString(payload.team_description, sizeof(payload.team_description), team.getDescription());
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_TEAMS_LIST, payload));
}

static void queueChannelInfo(CommandContext &context, const myteams::Channel &channel)
{
    myteams::PayloadRplChannel payload {};
    copyPaddedString(payload.channel_uuid, sizeof(payload.channel_uuid), channel.getUuid());
    copyPaddedString(payload.channel_name, sizeof(payload.channel_name), channel.getName());
    copyPaddedString(payload.channel_description, sizeof(payload.channel_description), channel.getDescription());
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CHANNELS_LIST, payload));
}

static void queueThreadInfo(CommandContext &context, const myteams::Thread &thread)
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
        buildPacket(myteams::RPL_THREADS_LIST, payload));
}

void handleInfoCommand(CommandContext &context)
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
        queueUserInfo(context, resolvedUser);
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
        queueTeamInfo(context, resolvedTeam);
        return;
    }
    const auto channel = findChannelByUuid(resolvedTeam, channelUuid);
    if (!channel.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Channel &resolvedChannel = channel->get();
    if (threadUuid.empty()) {
        queueChannelInfo(context, resolvedChannel);
        return;
    }
    const auto thread = findThreadByUuid(resolvedChannel, threadUuid);
    if (!thread.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    queueThreadInfo(context, thread->get());
}

} // namespace server::commands
