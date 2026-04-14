
#include "messages_command.hpp"
#include "commands/command_context.hpp"
#include "protocol.hpp"
#include "server/commands/command_utils.hpp"

namespace server::commands {

static void queueMessages(const std::vector<myteams::Message> &msgs, CommandContext &context) {
    for (const auto &msg: msgs) {
        auto payload = myteams::PayloadRplMessage{};
        copyPaddedString(payload.message_body, msg.getBody());
        copyPaddedString(payload.sender_uuid, msg.getAuthorUuid());
        payload.message_timestamp = static_cast<std::uint64_t>(msg.getCreatedAt());
        queuePacket(context.clientManager,
            context.clientFd,
            buildPacket(myteams::RPL_MESSAGES_LIST, payload));
    }
}

void handleMessagesCommand(CommandContext &context) {
    if (context.payloadSize != sizeof(myteams::PayloadReqTargetUser)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    if (context.authenticatedUsersByFd.find(context.clientFd) == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    myteams::PayloadReqTargetUser payload {};
    std::memcpy(&payload, context.payloadData.data(), sizeof(payload));

    std::vector<myteams::Message> filteredMessages;

    for (const auto& msg : context.messages) {
        if (msg.getAuthorUuid() == payload.target_uuid) {
            filteredMessages.push_back(msg);
        }
    }

    std::sort(filteredMessages.begin(), filteredMessages.end(),
        [](const myteams::Message& a, const myteams::Message& b) {
            return a.getCreatedAt() < b.getCreatedAt();
        }
    );

    queueMessages(filteredMessages, context);
    queueStatus(context, myteams::RPL_OK);
}

} // namespace server::commands
