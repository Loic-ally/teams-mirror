#include "send_command.hpp"
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
        throw std::runtime_error("invalid ERR_NOT_FOUND payload size for /send");
    }

    myteams::PayloadErrUnknown errorPayload {};
    std::memcpy(&errorPayload, payload.data(), sizeof(errorPayload));
    (void)Printer::errorUnknownUser(errorPayload.unknown_uuid);
}

} // namespace

void handleSend(Client &clientData, ParsedInput &input)
{
    const std::string targetUuid = input.getArg<std::string>();
    const std::string messageBody = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /send \"user_uuid\" \"message_body\"");
    }
    if (!isValidUuid(targetUuid)) {
        throw std::invalid_argument("Invalid UUID format.");
    }
    if (messageBody.empty()) {
        throw std::invalid_argument("Message body cannot be empty.");
    }

    myteams::PayloadReqSendMsg payload {};
    copyPaddedString(payload.target_uuid, sizeof(payload.target_uuid), targetUuid);
    copyPaddedString(payload.message_body, sizeof(payload.message_body), messageBody);

    const std::string packet = buildPacket(myteams::CMD_SEND, payload);
    sendPacket(*clientData.socket, packet);

    for (;;) {
        const ReceivedPacket reply = readReplySkippingEvents(*clientData.socket);
        if (reply.header.code == myteams::RPL_OK) {
            return;
        }
        if (reply.header.code == myteams::ERR_NOT_FOUND) {
            printUnknownUserError(reply.payload);
            throw std::runtime_error("Unknown user UUID");
        }
        if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
            (void)Printer::errorUnauthorized();
            throw std::runtime_error("Unauthorized: you must be logged in to use /send");
        }
        if (reply.header.code == myteams::ERR_BAD_REQUEST) {
            throw std::runtime_error("Server rejected /send request (bad request)");
        }
        throw std::runtime_error("unexpected reply code for /send");
    }
}

} // namespace client::commands
