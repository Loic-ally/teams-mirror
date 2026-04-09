#include "messages_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

namespace client::commands {
namespace {

struct ReceivedPacket {
    myteams::PacketHeader header {};
    std::string payload;
};

std::string readExact(const utils::Socket &socket, const std::size_t expectedSize)
{
    std::string buffer;
    buffer.reserve(expectedSize);

    while (buffer.size() < expectedSize) {
        const std::string chunk = socket.read(expectedSize - buffer.size());
        if (chunk.empty()) {
            continue;
        }
        buffer.append(chunk);
    }
    return buffer;
}

ReceivedPacket readPacket(const utils::Socket &socket)
{
    ReceivedPacket packet;
    const std::string rawHeader = readExact(socket, sizeof(myteams::PacketHeader));
    std::memcpy(&packet.header, rawHeader.data(), sizeof(packet.header));
    packet.payload = readExact(socket, packet.header.payload_size);
    return packet;
}

bool isEventCode(const std::uint16_t code)
{
    return code >= myteams::EVT_LOGGED_IN && code <= myteams::EVT_REPLY_CREATED;
}

void dispatchEventPacket(const ReceivedPacket &packet)
{
    if (packet.header.code == myteams::EVT_LOGGED_IN || packet.header.code == myteams::EVT_LOGGED_OUT) {
        if (packet.payload.size() != sizeof(myteams::PayloadEvtUserConnection)) {
            throw std::runtime_error("invalid user connection event payload size");
        }
        myteams::PayloadEvtUserConnection eventPayload {};
        std::memcpy(&eventPayload, packet.payload.data(), sizeof(eventPayload));
        if (packet.header.code == myteams::EVT_LOGGED_IN) {
            (void)Printer::eventLoggedIn(eventPayload.user_uuid, eventPayload.user_name);
        } else {
            (void)Printer::eventLoggedOut(eventPayload.user_uuid, eventPayload.user_name);
        }
        return;
    }

    if (packet.header.code == myteams::EVT_PRIVATE_MSG_RCVD) {
        if (packet.payload.size() != sizeof(myteams::PayloadEvtPrivateMsg)) {
            throw std::runtime_error("invalid private message event payload size");
        }
        myteams::PayloadEvtPrivateMsg eventPayload {};
        std::memcpy(&eventPayload, packet.payload.data(), sizeof(eventPayload));
        (void)Printer::eventPrivateMessageReceived(eventPayload.sender_uuid, eventPayload.message_body);
    }
}

ReceivedPacket readReplySkippingEvents(const utils::Socket &socket)
{
    for (;;) {
        ReceivedPacket packet = readPacket(socket);
        if (!isEventCode(packet.header.code)) {
            return packet;
        }
        dispatchEventPacket(packet);
    }
}

bool isHexCharacter(const char character)
{
    return std::isxdigit(static_cast<unsigned char>(character)) != 0;
}

bool isValidUuid(const std::string &uuid)
{
    if (uuid.size() != myteams::UUID_LENGTH - 1) {
        return false;
    }

    for (std::size_t index = 0; index < uuid.size(); ++index) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            if (uuid[index] != '-') {
                return false;
            }
            continue;
        }
        if (!isHexCharacter(uuid[index])) {
            return false;
        }
    }
    return true;
}

void printUnknownUserError(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadErrUnknown)) {
        throw std::runtime_error("invalid ERR_NOT_FOUND payload size for /messages");
    }

    myteams::PayloadErrUnknown errorPayload {};
    std::memcpy(&errorPayload, payload.data(), sizeof(errorPayload));
    (void)Printer::errorUnknownUser(errorPayload.unknown_uuid);
}

void printMessagesList(const std::string &payload)
{
    if (payload.size() % sizeof(myteams::PayloadRplMessage) != 0) {
        throw std::runtime_error("invalid RPL_MESSAGES_LIST payload size");
    }
    const std::size_t messageCount = payload.size() / sizeof(myteams::PayloadRplMessage);
    for (std::size_t index = 0; index < messageCount; ++index) {
        myteams::PayloadRplMessage messagePayload {};
        std::memcpy(
            &messagePayload,
            payload.data() + (index * sizeof(myteams::PayloadRplMessage)),
            sizeof(messagePayload));
        (void)Printer::printPrivateMessages(
            messagePayload.sender_uuid,
            static_cast<std::time_t>(messagePayload.message_timestamp),
            messagePayload.message_body);
    }
}

} // namespace

void handleMessages(Client &clientData, ParsedInput &input)
{
    const std::string targetUuid = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /messages \"user_uuid\"");
    }
    if (!isValidUuid(targetUuid)) {
        throw std::invalid_argument("Invalid UUID format.");
    }

    myteams::PayloadReqTargetUser payload {};
    copyPaddedString(payload.target_uuid, sizeof(payload.target_uuid), targetUuid);

    const std::string packet = buildPacket(myteams::CMD_MESSAGES, &payload, sizeof(payload));
    sendPacket(*clientData.socket, packet);

    for (;;) {
        const ReceivedPacket reply = readReplySkippingEvents(*clientData.socket);
        if (reply.header.code == myteams::RPL_OK) {
            return;
        }
        if (reply.header.code == myteams::RPL_MESSAGES_LIST) {
            printMessagesList(reply.payload);
            return;
        }
        if (reply.header.code == myteams::ERR_NOT_FOUND) {
            printUnknownUserError(reply.payload);
            throw std::runtime_error("Unknown user UUID");
        }
        if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
            (void)Printer::errorUnauthorized();
            throw std::runtime_error("Unauthorized: you must be logged in to use /messages");
        }
        if (reply.header.code == myteams::ERR_BAD_REQUEST) {
            throw std::runtime_error("Server rejected /messages request (bad request)");
        }
        throw std::runtime_error("unexpected reply code for /messages");
    }
}

} // namespace client::commands
