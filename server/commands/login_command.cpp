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
    std::memcpy(&payload, context.payloadData.data(), sizeof(payload));
    std::string requestedUserName;
    if (!extractFixedString(payload.user_name, sizeof(payload.user_name), requestedUserName)
        || requestedUserName.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    const auto completeLogin = [&context](myteams::User &user) {
        context.authenticatedUsersByFd[context.clientFd] = std::string(user.getUuid());
        context.authenticatedUsersByUUID[std::string(user.getUuid())].push_back(context.clientFd);
        ServerLogger::logUserLoggedIn(user.getUuid());
        queueStatus(context, myteams::RPL_OK);

        const std::string eventPacket =
            buildUserConnectionEventPacket(myteams::EVT_LOGGED_IN, user.getUuid(), user.getName());
        broadcastPacket(context, eventPacket);
    };

    const auto existingUser = findUserByName(context.users, requestedUserName);
    if (!existingUser.has_value()) {
        std::string userUuid = generateUuid();
        while (findUserByUuid(context.users, userUuid).has_value()) {
            userUuid = generateUuid();
        }
        context.users.emplace_back(userUuid, requestedUserName, true);
        myteams::User &user = context.users.back();
        ServerLogger::logUserCreated(user.getUuid(), user.getName());
        completeLogin(user);
        return;
    } else {
        myteams::User &user = existingUser->get();
        user.addLoggedIn();
        completeLogin(user);
    }
}

} // namespace server::commands

