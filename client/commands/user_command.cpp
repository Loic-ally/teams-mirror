#include "user_command.hpp"
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
namespace user_command_detail {

ReceivedPacket readReplySkippingEvents(const utils::Socket &socket)
{
    ReceivedPacket packet;
    readServerReply(socket, packet.header, packet.payload);
    return packet;
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

void printUserReply(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplUser)) {
        throw std::runtime_error("invalid RPL_USER_INFO payload size");
    }

    myteams::PayloadRplUser userPayload {};
    std::memcpy(&userPayload, payload.data(), sizeof(userPayload));
    Printer::printUser(
        userPayload.user_uuid,
        userPayload.user_name,
        static_cast<int>(userPayload.user_status));
}

void printUnknownUserError(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadErrUnknown)) {
        throw std::runtime_error("invalid ERR_NOT_FOUND payload size for /user");
    }

    myteams::PayloadErrUnknown errorPayload {};
    std::memcpy(&errorPayload, payload.data(), sizeof(errorPayload));
    Printer::errorUnknownUser(errorPayload.unknown_uuid);
}

} // namespace user_command_detail

void handleUser(Client &clientData, ParsedInput &input)
{
    const std::string targetUuid = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /user \"user_uuid\"");
    }
    if (!user_command_detail::isValidUuid(targetUuid)) {
        throw std::invalid_argument("Invalid UUID format.");
    }

    myteams::PayloadReqTargetUser payload {};
    copyPaddedString(payload.target_uuid, sizeof(payload.target_uuid), targetUuid);

    const std::string packet = buildPacket(myteams::CMD_USER_INFO, payload);
    sendPacket(*clientData.socket, packet);

    for (;;) {
        const auto reply = user_command_detail::readReplySkippingEvents(*clientData.socket);
        if (reply.header.code == myteams::RPL_OK) {
            continue;
        }
        if (reply.header.code == myteams::RPL_USER_INFO) {
            user_command_detail::printUserReply(reply.payload);
            return;
        }
        if (reply.header.code == myteams::ERR_NOT_FOUND) {
            user_command_detail::printUnknownUserError(reply.payload);
            throw std::runtime_error("Unknown user UUID");
        }
        if (reply.header.code == myteams::ERR_UNAUTHORIZED) {
            Printer::errorUnauthorized();
            throw std::runtime_error("Unauthorized: you must be logged in to use /user");
        }
        if (reply.header.code == myteams::ERR_BAD_REQUEST) {
            throw std::runtime_error("Server rejected /user request (bad request)");
        }
        throw std::runtime_error("unexpected reply code for /user");
    }
}

} // namespace client::commands
