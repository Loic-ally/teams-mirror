#include "server/commands/login_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/logger/server_logger.hpp"

#include <cstring>
#include <string>

namespace server::commands {

void handleLoginCommand(CommandContext &context)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqLogin)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    if (context.authenticatedUsersByFd.find(context.clientFd) != context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_ALREADY_EXIST);
        return;
    }

    myteams::PayloadReqLogin payload {};
    std::memcpy(&payload, context.payloadData, sizeof(payload));
    std::string requestedUserName;
    if (!extractFixedString(payload.user_name, sizeof(payload.user_name), requestedUserName)
        || requestedUserName.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    myteams::User *user = findUserByName(context.users, requestedUserName);
    if (user == nullptr) {
        std::string userUuid = generateUuid();
        while (findUserByUuid(context.users, userUuid) != nullptr) {
            userUuid = generateUuid();
        }
        context.users.emplace_back(userUuid, requestedUserName, true);
        user = &context.users.back();
        (void)ServerLogger::logUserCreated(user->getUuid(), user->getName());
    } else {
        if (user->isLoggedIn()) {
            queueStatus(context, myteams::ERR_ALREADY_EXIST);
            return;
        }
        user->setLoggedIn(true);
    }

    context.authenticatedUsersByFd[context.clientFd] = std::string(user->getUuid());
    (void)ServerLogger::logUserLoggedIn(user->getUuid());
    queueStatus(context, myteams::RPL_OK);

    const std::string eventPacket =
        buildUserConnectionEventPacket(myteams::EVT_LOGGED_IN, user->getUuid(), user->getName());
    broadcastPacket(context, eventPacket);
}

} // namespace server::commands

