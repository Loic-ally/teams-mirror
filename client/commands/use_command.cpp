#include "use_command.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "packet_utils.hpp"
#include "parser/parser.hpp"

#include <cstring>
#include <cctype>
#include <iostream>
#include <string>

namespace client::commands {
namespace {

bool isUuidFormatValid(const std::string &uuid)
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
        if (!std::isxdigit(static_cast<unsigned char>(uuid[index]))) {
            return false;
        }
    }
    return true;
}

std::string readExact(const utils::Socket &socket, const std::size_t wantedSize)
{
    std::string buffer;
    buffer.reserve(wantedSize);

    while (buffer.size() < wantedSize) {
        const std::string chunk = socket.read(wantedSize - buffer.size());
        buffer += chunk;
    }
    return buffer;
}

bool readServerPacket(
    const utils::Socket &socket,
    myteams::PacketHeader &outHeader,
    std::string &outPayload)
{
    const std::string headerBuffer = readExact(socket, sizeof(myteams::PacketHeader));
    std::memcpy(&outHeader, headerBuffer.data(), sizeof(outHeader));

    outPayload.clear();
    if (outHeader.payload_size == 0) {
        return true;
    }
    outPayload = readExact(socket, outHeader.payload_size);
    return true;
}

} // namespace

void handleUse(Client &clientData, ParsedInput &input)
{
    std::string teamUuid;
    std::string channelUuid;
    std::string threadUuid;

    if (input.hasRemainingArgs()) {
        teamUuid = input.getArg<std::string>();
        if (input.fail() || !isUuidFormatValid(teamUuid)) {
            std::cout << "Usage: /use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]" << std::endl;
            return;
        }
    }
    if (input.hasRemainingArgs()) {
        channelUuid = input.getArg<std::string>();
        if (input.fail() || !isUuidFormatValid(channelUuid)) {
            std::cout << "Usage: /use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]" << std::endl;
            return;
        }
    }
    if (input.hasRemainingArgs()) {
        threadUuid = input.getArg<std::string>();
        if (input.fail() || !isUuidFormatValid(threadUuid)) {
            std::cout << "Usage: /use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]" << std::endl;
            return;
        }
    }
    if (input.hasRemainingArgs()) {
        std::cout << "Usage: /use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]" << std::endl;
        return;
    }

    myteams::PayloadReqUse payload {};
    copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), teamUuid);
    copyPaddedString(payload.channel_uuid, sizeof(payload.channel_uuid), channelUuid);
    copyPaddedString(payload.thread_uuid, sizeof(payload.thread_uuid), threadUuid);
    sendPacket(*clientData.socket, buildPacket(myteams::CMD_USE, &payload, sizeof(payload)));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    (void)readServerPacket(*clientData.socket, responseHeader, responsePayload);
    if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
        std::cout << "You must be logged in to use this command." << std::endl;
        return;
    }
    if (responseHeader.code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid context payload sent to server." << std::endl;
        return;
    }
    if (responseHeader.code != myteams::RPL_OK) {
        std::cout << "Server returned unexpected status: " << responseHeader.code << std::endl;
        return;
    }

    clientData.contextTeamUuid = teamUuid;
    clientData.contextChannelUuid = channelUuid;
    clientData.contextThreadUuid = threadUuid;
}

} // namespace client::commands
