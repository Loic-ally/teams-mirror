#include "server/commands/use_command.hpp"
#include "server/commands/command_utils.hpp"

#include <cstring>
#include <string>

namespace server::commands {
namespace {

bool extractOptionalUuid(const char *rawData, const std::size_t rawSize, std::string &outValue)
{
    if (rawData == nullptr || rawSize == 0) {
        return false;
    }
    if (rawData[0] == '\0') {
        outValue.clear();
        return true;
    }
    if (!extractFixedString(rawData, rawSize, outValue)) {
        return false;
    }
    return isUuidFormatValid(outValue);
}

bool isContextCombinationValid(
    const std::string &teamUuid,
    const std::string &channelUuid,
    const std::string &threadUuid)
{
    if (teamUuid.empty() && (!channelUuid.empty() || !threadUuid.empty())) {
        return false;
    }
    if (channelUuid.empty() && !threadUuid.empty()) {
        return false;
    }
    return true;
}

} // namespace

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
    std::memcpy(&payload, context.payloadData, sizeof(payload));

    std::string teamUuid;
    std::string channelUuid;
    std::string threadUuid;
    if (!extractOptionalUuid(payload.team_uuid, sizeof(payload.team_uuid), teamUuid)
        || !extractOptionalUuid(payload.channel_uuid, sizeof(payload.channel_uuid), channelUuid)
        || !extractOptionalUuid(payload.thread_uuid, sizeof(payload.thread_uuid), threadUuid)
        || !isContextCombinationValid(teamUuid, channelUuid, threadUuid)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    context.clientManager.setContext(context.clientFd, teamUuid, channelUuid, threadUuid);
    queueStatus(context, myteams::RPL_OK);
}

} // namespace server::commands
