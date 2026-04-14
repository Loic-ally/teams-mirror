#include "server/commands/users_command.hpp"
#include "server/commands/command_utils.hpp"

#include <vector>

namespace server::commands {

namespace users_command_detail {

myteams::PayloadRplUser buildUserPayload(const myteams::User &user)
{
    myteams::PayloadRplUser payload {};
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), user.getUuid());
    copyPaddedString(payload.user_name, sizeof(payload.user_name), user.getName());
    payload.user_status = user.isLoggedIn() ? 1U : 0U;
    return payload;
}

} // namespace users_command_detail

void handleUsersCommand(CommandContext &context)
{
    if (context.payloadSize != 0) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    if (context.authenticatedUsersByFd.find(context.clientFd) == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }
    std::vector<myteams::PayloadRplUser> usersPayload;
    usersPayload.reserve(context.users.size());
    for (const myteams::User &user : context.users) {
        usersPayload.push_back(users_command_detail::buildUserPayload(user));
    }
    queuePacket(
        context.clientManager,
        context.clientFd,
        buildPacket(myteams::RPL_USERS_LIST, usersPayload));
}

} // namespace server::commands
