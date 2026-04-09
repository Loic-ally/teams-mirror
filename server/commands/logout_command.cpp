#include "server/commands/logout_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"
#include "server/core/logger/server_logger.hpp"

#include <string>

namespace server::commands {

namespace {

void broadcastLoggedOutEvent(
    ClientManager &clientManager,
    const ClientSockets &clientSockets,
    myteams::User &user,
    const std::int32_t excludedClientFd = -1)
{
    const std::string eventPacket =
        buildUserConnectionEventPacket(myteams::EVT_LOGGED_OUT, user.getUuid(), user.getName());
    broadcastPacket(clientManager, clientSockets, eventPacket, excludedClientFd);
}

} // namespace

void handleLogoutCommand(CommandContext &context)
{
    if (context.payloadSize != 0) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    const auto authenticatedUserIt = context.authenticatedUsersByFd.find(context.clientFd);
    if (authenticatedUserIt == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    myteams::User *user = findUserByUuid(context.users, authenticatedUserIt->second);
    context.authenticatedUsersByFd.erase(authenticatedUserIt);
    if (user == nullptr) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    user->setLoggedIn(false);
    (void)ServerLogger::logUserLoggedOut(user->getUuid());
    queueStatus(context, myteams::RPL_OK);
    broadcastLoggedOutEvent(context.clientManager, context.clientSockets, *user);
}

void handleClientDisconnection(
    ClientManager &clientManager,
    const std::int32_t clientFd,
    std::vector<myteams::User> &users,
    const ClientSockets &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd)
{
    const auto authenticatedUserIt = authenticatedUsersByFd.find(clientFd);
    if (authenticatedUserIt == authenticatedUsersByFd.end()) {
        return;
    }

    myteams::User *user = findUserByUuid(users, authenticatedUserIt->second);
    authenticatedUsersByFd.erase(authenticatedUserIt);
    if (user == nullptr) {
        return;
    }

    user->setLoggedIn(false);
    (void)ServerLogger::logUserLoggedOut(user->getUuid());
    broadcastLoggedOutEvent(clientManager, clientSockets, *user, clientFd);
}

} // namespace server::commands

