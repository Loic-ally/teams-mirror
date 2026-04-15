#include "commands/command_context.hpp"
#include "models/message/message.hpp"
#include "models/user/user.hpp"
#include "protocol.hpp"
#include "server/commands/command_utils.hpp"
#include "server/core/logger/server_logger.hpp"
#include "send_command.hpp"

namespace server::commands {

static void sendMessage(CommandContext &context, const myteams::Message &msg) {

    const auto &receiver = context.authenticatedUsersByUUID.find(std::string(msg.getReceiverUuid()));

    myteams::PayloadEvtPrivateMsg payload {};
    copyPaddedString(payload.message_body, msg.getBody());
    copyPaddedString(payload.sender_uuid, msg.getAuthorUuid());
    ServerLogger::logPrivateMessageSent(msg.getAuthorUuid(), msg.getReceiverUuid(), msg.getBody());
    if (receiver == context.authenticatedUsersByUUID.end()) {
        return;
    }
    queuePacket(context.clientManager,
        receiver->second,
        buildPacket(myteams::EVT_PRIVATE_MSG_RCVD, payload));
}

void handleSendCommand(CommandContext &context) {
    if (context.payloadSize != sizeof(myteams::PayloadReqSendMsg)) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }

    const auto &client = context.authenticatedUsersByFd.find(context.clientFd);
    if (client == context.authenticatedUsersByFd.end()) {
        queueStatus(context, myteams::ERR_UNAUTHORIZED);
        return;
    }

    myteams::PayloadReqSendMsg payload {};
    std::memcpy(&payload, context.payloadData.data(), sizeof(payload));

    std::string body;
    std::string targetUUID;

    extractFixedString(payload.message_body, body);
    extractFixedString(payload.target_uuid, targetUUID);

    bool userExists = std::any_of(context.users.begin(), context.users.end(),
    [&targetUUID](const myteams::User& user) {
        return user.getUuid() == targetUUID;
    });

    if (!userExists) {
        queueStatus(context, myteams::ERR_NOT_FOUND);
        return;
    }

    const auto &msg = context.messages.emplace_back(
        generateUuid(),
        client->second,
        std::time(nullptr),
        body,
        targetUUID);
    sendMessage(context, msg);
    queueStatus(context, myteams::RPL_OK);
}

} // namespace server::commands
