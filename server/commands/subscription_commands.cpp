#include "server/commands/subscription_commands.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/logger/server_logger.hpp"

#include <cstring>
#include <string>
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

bool parseTeamUuidFromPayload(CommandContext &context, std::string &outTeamUuid)
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

void queueTeamsList(CommandContext &context, const myteams::User &authenticatedUser)
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

void queueUsersList(CommandContext &context, const myteams::Team &team)
{
    std::vector<myteams::PayloadRplUser> payloads;

    for (const myteams::Team::UserUuid &subscribedUserUuid : team.getSubscribedUsers()) {
        const std::string_view userUuid(subscribedUserUuid.data());
        myteams::User *user = findUserByUuid(context.users, userUuid);
        if (user == nullptr) {
            continue;
        }
        myteams::PayloadRplUser payload {};
        copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), user->getUuid());
        copyPaddedString(payload.user_name, sizeof(payload.user_name), user->getName());
        payload.user_status = user->isLoggedIn() ? 1U : 0U;
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

} // namespace

void handleSubscribeCommand(CommandContext &context)
{
    myteams::User *authenticatedUser = getAuthenticatedUser(context);
    if (authenticatedUser == nullptr) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    myteams::Team *team = findTeamByUuid(context.teams, teamUuid);
    if (team == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    if (team->isUserSubscribed(authenticatedUser->getUuid())) {
        queueStatus(context, myteams::ERR_ALREADY_EXIST);
        return;
    }

    team->addSubscribedUser(std::string(authenticatedUser->getUuid()));
    (void)ServerLogger::logUserSubscribed(team->getUuid(), authenticatedUser->getUuid());
    queueStatus(context, myteams::RPL_OK);
}

void handleUnsubscribeCommand(CommandContext &context)
{
    myteams::User *authenticatedUser = getAuthenticatedUser(context);
    if (authenticatedUser == nullptr) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    myteams::Team *team = findTeamByUuid(context.teams, teamUuid);
    if (team == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }
    if (!team->removeSubscribedUser(authenticatedUser->getUuid())) {
        queueStatus(context, myteams::ERR_FORBIDDEN);
        return;
    }

    (void)ServerLogger::logUserUnsubscribed(team->getUuid(), authenticatedUser->getUuid());
    queueStatus(context, myteams::RPL_OK);
}

void handleSubscribedListCommand(CommandContext &context)
{
    myteams::User *authenticatedUser = getAuthenticatedUser(context);
    if (authenticatedUser == nullptr) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    if (context.payloadSize == 0) {
        queueTeamsList(context, *authenticatedUser);
        return;
    }

    std::string teamUuid;
    if (!parseTeamUuidFromPayload(context, teamUuid)) {
        return;
    }

    myteams::Team *team = findTeamByUuid(context.teams, teamUuid);
    if (team == nullptr) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    queueUsersList(context, *team);
}

} // namespace server::commands
