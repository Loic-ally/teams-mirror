#include "server/commands/logout_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"
#include "server/core/logger/server_logger.hpp"

#include <algorithm>
#include <string>

namespace server::commands {

static void broadcastLoggedOutEvent(
    ClientManager &clientManager,
    const AuthenticatedUserByFd &authenticatedUsersByFd,
    myteams::User &user,
    const std::int32_t excludedClientFd = -1)
{
    const std::string eventPacket =
        buildUserConnectionEventPacket(myteams::EVT_LOGGED_OUT, user.getUuid(), user.getName());
    for (const auto &[socketFd, _] : authenticatedUsersByFd) {
        if (socketFd == excludedClientFd) {
            continue;
        }
        queuePacket(clientManager, socketFd, eventPacket);
    }
}

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

    const auto user = findUserByUuid(context.users, authenticatedUserIt->second);
    auto &listSocket = context.authenticatedUsersByUUID[authenticatedUserIt->second];
    listSocket.erase(
        std::remove(listSocket.begin(), listSocket.end(), context.clientFd),
        listSocket.end());
    context.authenticatedUsersByFd.erase(authenticatedUserIt);
    if (!user.has_value()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    myteams::User &resolvedUser = user->get();

    resolvedUser.removeLoggedIn();
    ServerLogger::logUserLoggedOut(resolvedUser.getUuid());
    queueStatus(context, myteams::RPL_OK);
    broadcastLoggedOutEvent(context.clientManager, context.authenticatedUsersByFd, resolvedUser);
}

void handleClientDisconnection(
    ClientManager &clientManager,
    const std::int32_t clientFd,
    std::vector<myteams::User> &users,
    AuthenticatedUserByFd &authenticatedUsersByFd,
    AuthenticatedUserByUUID &authenticatedUsersByUUID
)
{
    const auto authenticatedUserIt = authenticatedUsersByFd.find(clientFd);
    if (authenticatedUserIt == authenticatedUsersByFd.end()) {
        return;
    }
    const auto user = findUserByUuid(users, authenticatedUserIt->second);
    auto &listSocket = authenticatedUsersByUUID[authenticatedUserIt->second];
    const auto authenticatedUserUUIDIt = authenticatedUsersByUUID.find(authenticatedUserIt->second);
    listSocket.erase(
        std::remove(listSocket.begin(), listSocket.end(), clientFd),
        listSocket.end());
    authenticatedUsersByFd.erase(authenticatedUserIt);
    if (authenticatedUserUUIDIt == authenticatedUsersByUUID.end()) {
        return;
    }
    authenticatedUsersByUUID.erase(authenticatedUserUUIDIt);
    if (!user.has_value()) {
        return;
    }
    myteams::User &resolvedUser = user->get();
    resolvedUser.removeLoggedIn();
    ServerLogger::logUserLoggedOut(resolvedUser.getUuid());
    broadcastLoggedOutEvent(clientManager, authenticatedUsersByFd, resolvedUser, clientFd);
}

} // namespace server::commands

