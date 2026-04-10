#include "server/commands/use_command.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <cstring>
#include <string>
#include <string_view>

namespace server::commands {

static bool extractOptionalUuid(const std::string_view rawData, std::string &outValue)
{
    if (rawData.empty()) {
        return false;
    }
    if (rawData.front() == '\0') {
        outValue.clear();
        return true;
    }
    if (!extractFixedString(rawData, outValue)) {
        return false;
    }
    return isUuidFormatValid(outValue);
}

void handleUseCommand(CommandContext &context)
{
    if (context.payloadSize != sizeof(myteams::PayloadReqUse)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    if (context.authenticatedUsersByFd.find(context.clientFd) == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    myteams::PayloadReqUse payload {};
    std::memcpy(&payload, context.payloadData.data(), sizeof(payload));

    std::string teamUuid;
    std::string channelUuid;
    std::string threadUuid;
    if (!extractOptionalUuid(std::string_view(payload.team_uuid, sizeof(payload.team_uuid)), teamUuid)
        || !extractOptionalUuid(std::string_view(payload.channel_uuid, sizeof(payload.channel_uuid)), channelUuid)
        || !extractOptionalUuid(std::string_view(payload.thread_uuid, sizeof(payload.thread_uuid)), threadUuid)
        || !isContextCombinationValid(teamUuid, channelUuid, threadUuid)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    context.clientManager.setContext(context.clientFd, teamUuid, channelUuid, threadUuid);
    queueStatus(context, myteams::RPL_OK);
}

} // namespace server::commands
