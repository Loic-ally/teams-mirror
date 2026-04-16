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
namespace users_command_detail {

ReceivedPacket readReplySkippingEvents(const utils::Socket &socket)
{
    ReceivedPacket packet;
    readServerReply(socket, packet.header, packet.payload);
    return packet;
}

void printUsersList(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplUser)) {
        throw std::runtime_error("invalid RPL_USERS_LIST payload size");
    }
    myteams::PayloadRplUser userPayload {};
    std::memcpy(&userPayload, payload.data(), sizeof(userPayload));
    Printer::printUsers(
        userPayload.user_uuid,
        userPayload.user_name,
        static_cast<int>(userPayload.user_status));
}

} // namespace users_command_detail

void handleUsers(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /users");
    }
    const std::string packet = buildPacket(myteams::CMD_USERS);
    sendPacket(*clientData.socket, packet);
    for (;;) {
        const auto reply = users_command_detail::readReplySkippingEvents(*clientData.socket);
        if (reply.header.code == myteams::RPL_OK) {
            return;
        }
        if (reply.header.code == myteams::RPL_USERS_LIST) {
            users_command_detail::printUsersList(reply.payload);
            continue;
        }
        if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
            Printer::errorUnauthorized();
            throw std::runtime_error("Unauthorized: you must be logged in to use /users");
        }
        if (reply.header.code == myteams::ERR_BAD_REQUEST) {
            throw std::runtime_error("Server rejected /users request (bad request)");
        }
        throw std::runtime_error("unexpected reply code for /users");
    }
}

} // namespace client::commands
