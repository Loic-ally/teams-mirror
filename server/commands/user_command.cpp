#include "server/commands/user_command.hpp"
#include "server/commands/command_utils.hpp"

#include <cstring>

namespace server::commands {

namespace {

myteams::PayloadRplUser buildUserPayload(const myteams::User &user)
{
    myteams::PayloadRplUser payload {};
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), user.getUuid());
    copyPaddedString(payload.user_name, sizeof(payload.user_name), user.getName());
    payload.user_status = user.isLoggedIn() ? 1U : 0U;
    return payload;
}

} // namespace

void handleUserCommand(CommandContext &context)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqTargetUser)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    if (context.authenticatedUsersByFd.find(context.clientFd) == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    myteams::PayloadReqTargetUser requestPayload {};
    std::memcpy(&requestPayload, context.payloadData, sizeof(requestPayload));
    std::string requestedUserUuid;
    if (!extractFixedString(
            requestPayload.target_uuid,
            sizeof(requestPayload.target_uuid),
            requestedUserUuid)
        || requestedUserUuid.empty()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    myteams::User *user = findUserByUuid(context.users, requestedUserUuid);
    if (user == nullptr) {
        myteams::PayloadErrUnknown errorPayload {};
        copyPaddedString(
            errorPayload.unknown_uuid,
            sizeof(errorPayload.unknown_uuid),
            requestedUserUuid);
        queuePacket(
            context.clientManager,
            context.clientFd,
            buildPacket(myteams::ERR_NOT_FOUND, &errorPayload, sizeof(errorPayload)));
        return;
    }
    const myteams::PayloadRplUser responsePayload = buildUserPayload(*user);
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_USER_INFO, &responsePayload, sizeof(responsePayload)));
}

} // namespace server::commands
