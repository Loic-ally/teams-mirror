#include "server/commands/subscription_commands.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/logger/server_logger.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace server::commands {

static bool parseTeamUuidFromPayload(CommandContext &context, std::string &outTeamUuid)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqTeamTarget)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }

    myteams::PayloadReqTeamTarget payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));
    if (!extractFixedString(payload.team_uuid, sizeof(payload.team_uuid), outTeamUuid)
        || !isUuidFormatValid(outTeamUuid)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return false;
    }
    return true;
}

static void queueTeamsList(CommandContext &context, const myteams::User &authenticatedUser)
{
    std::vector<myteams::PayloadRplTeam> payloads;

    for (const myteams::Team &team : context.teams) {
        if (!team.isUserSubscribed(authenticatedUser.getUuid())) {
            continue;
        }
        myteams::PayloadRplTeam payload {};
        copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), team.getUuid());
        copyPaddedString(payload.team_name, sizeof(payload.team_name), team.getName());
        copyPaddedString(payload.team_description, sizeof(payload.team_description), team.getDescription());
        payloads.push_back(payload);
    }

    if (payloads.empty()) {
        queuePacket(
            context.clientManager,
            context.clientFd,
            buildPacket(myteams::RPL_TEAMS_LIST));
        return;
    }

    const std::uint16_t payloadSize = static_cast<std::uint16_t>(payloads.size() * sizeof(myteams::PayloadRplTeam));
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_TEAMS_LIST, payloads.data(), payloadSize));
}

static void queueUsersList(CommandContext &context, const myteams::Team &team)
{
    std::vector<myteams::PayloadRplUser> payloads;

    for (const myteams::Team::UserUuid &subscribedUserUuid : team.getSubscribedUsers()) {
        const std::string_view userUuid(subscribedUserUuid.data());
        const auto user = findUserByUuid(context.users, userUuid);
        if (!user.has_value()) {
            continue;
        }
        const myteams::User &resolvedUser = user->get();
        myteams::PayloadRplUser payload {};
        copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), resolvedUser.getUuid());
        copyPaddedString(payload.user_name, sizeof(payload.user_name), resolvedUser.getName());
        payload.user_status = resolvedUser.isLoggedIn() ? 1U : 0U;
        payloads.push_back(payload);
    }

    if (payloads.empty()) {
        queuePacket(
            context.clientManager,
            context.clientFd,
            buildPacket(myteams::RPL_USERS_LIST));
        return;
    }

    const std::uint16_t payloadSize = static_cast<std::uint16_t>(payloads.size() * sizeof(myteams::PayloadRplUser));
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_USERS_LIST, payloads.data(), payloadSize));
}

void handleSubscribeCommand(CommandContext &context)
{
    const auto authenticatedUser = getAuthenticatedUser(context);
    if (!authenticatedUser.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    myteams::User &resolvedUser = authenticatedUser->get();

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Team &resolvedTeam = team->get();
    if (resolvedTeam.isUserSubscribed(resolvedUser.getUuid())) {
        queueStatus(context, myteams::ERR_ALREADY_EXIST);
        return;
    }

    resolvedTeam.addSubscribedUser(std::string(resolvedUser.getUuid()));
    ServerLogger::logUserSubscribed(resolvedTeam.getUuid(), resolvedUser.getUuid());
    queueStatus(context, myteams::RPL_OK);
}

void handleUnsubscribeCommand(CommandContext &context)
{
    const auto authenticatedUser = getAuthenticatedUser(context);
    if (!authenticatedUser.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    myteams::User &resolvedUser = authenticatedUser->get();

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    myteams::Team &resolvedTeam = team->get();
    if (!resolvedTeam.removeSubscribedUser(resolvedUser.getUuid())) {
        queueStatus(context, myteams::ERR_FORBIDDEN);
        return;
    }

    ServerLogger::logUserUnsubscribed(resolvedTeam.getUuid(), resolvedUser.getUuid());
    queueStatus(context, myteams::RPL_OK);
}

void handleSubscribedListCommand(CommandContext &context)
{
    const auto authenticatedUser = getAuthenticatedUser(context);
    if (!authenticatedUser.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    const myteams::User &resolvedUser = authenticatedUser->get();

    if (context.payloadSize == 0) {
        queueTeamsList(context, resolvedUser);
        return;
    }

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    const auto team = findTeamByUuid(context.teams, teamUuid);
    if (!team.has_value()) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    queueUsersList(context, team->get());
}

} // namespace server::commands
