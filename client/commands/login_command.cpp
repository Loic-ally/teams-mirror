#include "login_command.hpp"
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

} // namespace

void handleLogin(Client &clientData, ParsedInput &input)
{
    if (clientData.connected) {
        throw std::logic_error(
            "You are already logged in as \"" + clientData.username + "\". Use /logout first.");
    }

    const std::string username = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /login \"user_name\"");
    }
    if (username.empty()) {
        throw std::invalid_argument("Username cannot be empty.");
    }

    myteams::PayloadReqLogin payload {};
    copyPaddedString(payload.user_name, sizeof(payload.user_name), username);
    const std::string packet =
        buildPacket(myteams::CMD_LOGIN, &payload, sizeof(payload));
    sendPacket(*clientData.socket, packet);

    const ReceivedPacket reply = readReplySkippingEvents(*clientData.socket);
    if (reply.header.code == myteams::RPL_OK) {
        clientData.connected = true;
        clientData.username = username;
        (void)Printer::eventLoggedIn("", clientData.username);
        return;
    }
    if (reply.header.code == myteams::ERR_ALREADY_EXIST) {
        (void)Printer::errorAlreadyExist();
        throw std::runtime_error("Login refused: user is already logged in");
    }
    if (reply.header.code == myteams::ERR_BAD_REQUEST) {
        throw std::runtime_error("Server rejected /login request (bad request)");
    }
    if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
        throw std::runtime_error("Server rejected /login request (unauthorized)");
    }
    throw std::runtime_error("unexpected reply code for /login");
}

}
