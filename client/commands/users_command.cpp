#include "users_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

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

void printUsersList(const std::string &payload)
{
    if (payload.size() % sizeof(myteams::PayloadRplUser) != 0) {
        throw std::runtime_error("invalid RPL_USERS_LIST payload size");
    }
    const std::size_t userCount = payload.size() / sizeof(myteams::PayloadRplUser);
    for (std::size_t index = 0; index < userCount; ++index) {
        myteams::PayloadRplUser userPayload {};
        std::memcpy(
            &userPayload,
            payload.data() + (index * sizeof(myteams::PayloadRplUser)),
            sizeof(userPayload));
        (void)Printer::printUsers(
            userPayload.user_uuid,
            userPayload.user_name,
            static_cast<int>(userPayload.user_status));
    }
}

} // namespace

void handleUsers(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /users");
    }
    const std::string packet = buildPacket(myteams::CMD_USERS);
    sendPacket(*clientData.socket, packet);
    for (;;) {
        const ReceivedPacket reply = readReplySkippingEvents(*clientData.socket);
        if (reply.header.code == myteams::RPL_OK) {
            continue;
        }
        if (reply.header.code == myteams::RPL_USERS_LIST) {
            printUsersList(reply.payload);
            return;
        }
        if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
            (void)Printer::errorUnauthorized();
            throw std::runtime_error("Unauthorized: you must be logged in to use /users");
        }
        if (reply.header.code == myteams::ERR_BAD_REQUEST) {
            throw std::runtime_error("Server rejected /users request (bad request)");
        }
        throw std::runtime_error("unexpected reply code for /users");
    }
}

} // namespace client::commands
