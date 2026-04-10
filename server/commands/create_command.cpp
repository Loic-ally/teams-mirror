#include "server/commands/create_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"
#include "server/core/logger/server_logger.hpp"

#include <cstdint>
#include <ctime>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace server::commands {

using TeamChannel = std::pair<TeamRef, ChannelRef>;

static bool isChannelUuidUsed(const std::vector<myteams::Team> &teams, const std::string_view channelUuid)
{
    for (const myteams::Team &team : teams) {
        for (const myteams::Channel &channel : team.getChannels()) {
            if (channel.getUuid() == channelUuid) {
                return true;
            }
        }
    }
    return false;
}

static bool isThreadUuidUsed(const std::vector<myteams::Team> &teams, const std::string_view threadUuid)
{
    for (const myteams::Team &team : teams) {
        for (const myteams::Channel &channel : team.getChannels()) {
            for (const myteams::Thread &thread : channel.getThreads()) {
                if (thread.getUuid() == threadUuid) {
                    return true;
                }
            }
        }
    }
    return false;
}

static std::string generateUniqueTeamUuid(std::vector<myteams::Team> &teams)
{
    std::string candidate = generateUuid();
    while (findTeamByUuid(teams, candidate).has_value()) {
        candidate = generateUuid();
    }
    return candidate;
}

static std::string generateUniqueChannelUuid(const std::vector<myteams::Team> &teams)
{
    std::string candidate = generateUuid();
    while (isChannelUuidUsed(teams, candidate)) {
        candidate = generateUuid();
    }
    return candidate;
}

static std::string generateUniqueThreadUuid(const std::vector<myteams::Team> &teams)
{
    std::string candidate = generateUuid();
    while (isThreadUuidUsed(teams, candidate)) {
        candidate = generateUuid();
    }
    return candidate;
}

static bool parseCreateTeamPayload(CommandContext &context, std::string &outName, std::string &outDescription)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqCreateTeam)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    myteams::PayloadReqCreateTeam payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));

    if (!extractFixedString(payload.team_name, sizeof(payload.team_name), outName)
        || !extractFixedString(payload.team_description, sizeof(payload.team_description), outDescription)
        || outName.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    return true;
}

static bool parseCreateChannelPayload(CommandContext &context, std::string &outName, std::string &outDescription)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqCreateChannel)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    myteams::PayloadReqCreateChannel payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));

    if (!extractFixedString(payload.channel_name, sizeof(payload.channel_name), outName)
        || !extractFixedString(payload.channel_description, sizeof(payload.channel_description), outDescription)
        || outName.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    return true;
}

static bool parseCreateThreadPayload(CommandContext &context, std::string &outTitle, std::string &outBody)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqCreateThread)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    myteams::PayloadReqCreateThread payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));

    if (!extractFixedString(payload.thread_title, sizeof(payload.thread_title), outTitle)
        || !extractFixedString(payload.thread_body, sizeof(payload.thread_body), outBody)
        || outTitle.empty()
        || outBody.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    return true;
}

static bool parseCreateReplyPayload(CommandContext &context, std::string &outBody)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqCreateReply)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    myteams::PayloadReqCreateReply payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));

    if (!extractFixedString(payload.reply_body, sizeof(payload.reply_body), outBody) || outBody.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    return true;
}

static void queuePacketToTeamSubscribers(CommandContext &context, const myteams::Team &team, const std::string &packet)
{
    for (const auto &[socketFd, userUuid] : context.authenticatedUsersByFd) {
        if (!team.isUserSubscribed(userUuid)) {
            continue;
        }
        queuePacket(context.clientManager, socketFd, packet);
    }
}

static void handleCreateTeam(CommandContext &context, myteams::User &authenticatedUser)
{
    std::string teamName;
    std::string teamDescription;
    if (!parseCreateTeamPayload(context, teamName, teamDescription)) {
        return;
    }

    const std::string teamUuid = generateUniqueTeamUuid(context.teams);
    context.teams.emplace_back(teamUuid, teamName, teamDescription);
    myteams::Team &createdTeam = context.teams.back();
    createdTeam.addSubscribedUser(std::string(authenticatedUser.getUuid()));

    ServerLogger::logTeamCreated(createdTeam.getUuid(), createdTeam.getName(), authenticatedUser.getUuid());

    myteams::PayloadRplTeam createdPayload {};
    copyPaddedString(createdPayload.team_uuid, sizeof(createdPayload.team_uuid), createdTeam.getUuid());
    copyPaddedString(createdPayload.team_name, sizeof(createdPayload.team_name), createdTeam.getName());
    copyPaddedString(createdPayload.team_description, sizeof(createdPayload.team_description), createdTeam.getDescription());
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CREATED, &createdPayload, sizeof(createdPayload)));

    myteams::PayloadEvtTeamCreated eventPayload {};
    copyPaddedString(eventPayload.team_uuid, sizeof(eventPayload.team_uuid), createdTeam.getUuid());
    copyPaddedString(eventPayload.team_name, sizeof(eventPayload.team_name), createdTeam.getName());
    copyPaddedString(eventPayload.team_description, sizeof(eventPayload.team_description), createdTeam.getDescription());
    broadcastPacket(
        context,
        buildPacket(myteams::EVT_TEAM_CREATED, &eventPayload, sizeof(eventPayload)));
}

static std::optional<TeamChannel> resolveContextTeamChannel(
    CommandContext &context,
    const myteams::User &authenticatedUser)
{
    const std::string teamUuid = context.clientManager.getContextTeamUuid(context.clientFd);
    const std::string channelUuid = context.clientManager.getContextChannelUuid(context.clientFd);

    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return std::nullopt;
    }
    if (!team->get().isUserSubscribed(authenticatedUser.getUuid())) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return std::nullopt;
    }

    const auto channel = findChannelByUuid(team->get(), channelUuid);
    if (!channel.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return std::nullopt;
    }
    return TeamChannel(*team, *channel);
}

static void handleCreateChannel(CommandContext &context, myteams::User &authenticatedUser)
{
    std::string channelName;
    std::string channelDescription;
    if (!parseCreateChannelPayload(context, channelName, channelDescription)) {
        return;
    }

    const std::string teamUuid = context.clientManager.getContextTeamUuid(context.clientFd);
    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Team &resolvedTeam = team->get();
    if (!resolvedTeam.isUserSubscribed(authenticatedUser.getUuid())) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    const std::string channelUuid = generateUniqueChannelUuid(context.teams);
    resolvedTeam.addChannel(myteams::Channel(channelUuid, channelName, channelDescription));

    ServerLogger::logChannelCreated(resolvedTeam.getUuid(), channelUuid, channelName);

    myteams::PayloadRplChannel createdPayload {};
    copyPaddedString(createdPayload.channel_uuid, sizeof(createdPayload.channel_uuid), channelUuid);
    copyPaddedString(createdPayload.channel_name, sizeof(createdPayload.channel_name), channelName);
    copyPaddedString(createdPayload.channel_description, sizeof(createdPayload.channel_description), channelDescription);
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CREATED, &createdPayload, sizeof(createdPayload)));

    myteams::PayloadEvtChannelCreated eventPayload {};
    copyPaddedString(eventPayload.channel_uuid, sizeof(eventPayload.channel_uuid), channelUuid);
    copyPaddedString(eventPayload.channel_name, sizeof(eventPayload.channel_name), channelName);
    copyPaddedString(eventPayload.channel_description, sizeof(eventPayload.channel_description), channelDescription);
    queuePacketToTeamSubscribers(
        context,
        resolvedTeam,
        buildPacket(myteams::EVT_CHANNEL_CREATED, &eventPayload, sizeof(eventPayload)));
}

static void handleCreateThread(CommandContext &context, myteams::User &authenticatedUser)
{
    std::string threadTitle;
    std::string threadBody;
    if (!parseCreateThreadPayload(context, threadTitle, threadBody)) {
        return;
    }

    const auto teamChannel = resolveContextTeamChannel(context, authenticatedUser);
    if (!teamChannel.has_value()) {
        return;
    }
    myteams::Team &team = teamChannel->first.get();
    myteams::Channel &channel = teamChannel->second.get();

    const std::string threadUuid = generateUniqueThreadUuid(context.teams);
    const std::time_t now = std::time(nullptr);
    channel.addThread(myteams::Thread(threadUuid, std::string(authenticatedUser.getUuid()), now, threadTitle, threadBody));

    ServerLogger::logThreadCreated(channel.getUuid(), threadUuid, authenticatedUser.getUuid(), threadTitle, threadBody);

    myteams::PayloadRplThread createdPayload {};
    copyPaddedString(createdPayload.thread_uuid, sizeof(createdPayload.thread_uuid), threadUuid);
    copyPaddedString(createdPayload.user_uuid, sizeof(createdPayload.user_uuid), authenticatedUser.getUuid());
    createdPayload.thread_timestamp = static_cast<std::uint64_t>(now);
    copyPaddedString(createdPayload.thread_title, sizeof(createdPayload.thread_title), threadTitle);
    copyPaddedString(createdPayload.thread_body, sizeof(createdPayload.thread_body), threadBody);
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CREATED, &createdPayload, sizeof(createdPayload)));

    myteams::PayloadEvtThreadCreated eventPayload {};
    copyPaddedString(eventPayload.thread_uuid, sizeof(eventPayload.thread_uuid), threadUuid);
    copyPaddedString(eventPayload.user_uuid, sizeof(eventPayload.user_uuid), authenticatedUser.getUuid());
    eventPayload.thread_timestamp = static_cast<std::uint64_t>(now);
    copyPaddedString(eventPayload.thread_title, sizeof(eventPayload.thread_title), threadTitle);
    copyPaddedString(eventPayload.thread_body, sizeof(eventPayload.thread_body), threadBody);
    queuePacketToTeamSubscribers(
        context,
        team,
        buildPacket(myteams::EVT_THREAD_CREATED, &eventPayload, sizeof(eventPayload)));
}

static void handleCreateReply(CommandContext &context, myteams::User &authenticatedUser)
{
    std::string replyBody;
    if (!parseCreateReplyPayload(context, replyBody)) {
        return;
    }

    const auto teamChannel = resolveContextTeamChannel(context, authenticatedUser);
    if (!teamChannel.has_value()) {
        return;
    }
    myteams::Team &team = teamChannel->first.get();
    myteams::Channel &channel = teamChannel->second.get();

    const std::string threadUuid = context.clientManager.getContextThreadUuid(context.clientFd);
    const auto thread = findThreadByUuid(channel, threadUuid);
    if (!thread.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Thread &resolvedThread = thread->get();

    const std::time_t now = std::time(nullptr);
    resolvedThread.addReply(myteams::Message(generateUuid(), std::string(authenticatedUser.getUuid()), now, replyBody));

    ServerLogger::logReplyCreated(resolvedThread.getUuid(), authenticatedUser.getUuid(), replyBody);

    myteams::PayloadRplReply createdPayload {};
    copyPaddedString(createdPayload.thread_uuid, sizeof(createdPayload.thread_uuid), resolvedThread.getUuid());
    copyPaddedString(createdPayload.user_uuid, sizeof(createdPayload.user_uuid), authenticatedUser.getUuid());
    createdPayload.reply_timestamp = static_cast<std::uint64_t>(now);
    copyPaddedString(createdPayload.reply_body, sizeof(createdPayload.reply_body), replyBody);
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_CREATED, &createdPayload, sizeof(createdPayload)));

    myteams::PayloadEvtReplyCreated eventPayload {};
    copyPaddedString(eventPayload.team_uuid, sizeof(eventPayload.team_uuid), team.getUuid());
    copyPaddedString(eventPayload.thread_uuid, sizeof(eventPayload.thread_uuid), resolvedThread.getUuid());
    copyPaddedString(eventPayload.user_uuid, sizeof(eventPayload.user_uuid), authenticatedUser.getUuid());
    copyPaddedString(eventPayload.reply_body, sizeof(eventPayload.reply_body), replyBody);
    queuePacketToTeamSubscribers(
        context,
        team,
        buildPacket(myteams::EVT_REPLY_CREATED, &eventPayload, sizeof(eventPayload)));
}

void handleCreateCommand(CommandContext &context)
{
    const auto authenticatedUser = getAuthenticatedUser(context);
    if (!authenticatedUser.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    myteams::User &resolvedUser = authenticatedUser->get();

    const std::string teamUuid = context.clientManager.getContextTeamUuid(context.clientFd);
    const std::string channelUuid = context.clientManager.getContextChannelUuid(context.clientFd);
    const std::string threadUuid = context.clientManager.getContextThreadUuid(context.clientFd);

    const bool hasTeam = !teamUuid.empty();
    const bool hasChannel = !channelUuid.empty();
    const bool hasThread = !threadUuid.empty();

    if (!hasTeam && (hasChannel || hasThread)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    if (!hasChannel && hasThread) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    if (!hasTeam) {
        handleCreateTeam(context, resolvedUser);
        return;
    }
    if (!hasChannel) {
        handleCreateChannel(context, resolvedUser);
        return;
    }
    if (!hasThread) {
        handleCreateThread(context, resolvedUser);
        return;
    }
    handleCreateReply(context, resolvedUser);
}

} // namespace server::commands
