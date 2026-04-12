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

ReceivedPacket readReplySkippingEvents(const utils::Socket &socket)
{
    ReceivedPacket packet;
    readServerReply(socket, packet.header, packet.payload);
    return packet;
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
