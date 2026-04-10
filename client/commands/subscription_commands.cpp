#include "subscription_commands.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cctype>
#include <cstring>
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

void handleCommonError(std::uint16_t statusCode, const std::string &teamUuid)
{
    if (statusCode == myteams::ERR_UNAUTHORIZED) {
        (void)Printer::errorUnauthorized();
        return;
    }
    if (statusCode == myteams::ERR_NOT_FOUND) {
        (void)Printer::errorUnknownTeam(teamUuid);
        return;
    }
    if (statusCode == myteams::ERR_ALREADY_EXIST) {
        (void)Printer::errorAlreadyExist();
        return;
    }
    std::cout << "Server returned unexpected status: " << statusCode << std::endl;
}

} // namespace

void handleSubscribe(Client &clientData, ParsedInput &input)
{
    const std::string teamUuid = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        std::cout << "Usage: /subscribe \"team_uuid\"" << std::endl;
        return;
    }
    if (!isUuidFormatValid(teamUuid)) {
        std::cout << "Invalid team UUID format." << std::endl;
        return;
    }

    myteams::PayloadReqTeamTarget payload {};
    copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), teamUuid);
    sendPacket(
        *clientData.socket,
        buildPacket(myteams::CMD_SUBSCRIBE, &payload, sizeof(payload)));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    (void)readServerPacket(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        (void)Printer::printSubscribed("", teamUuid);
        return;
    }
    handleCommonError(responseHeader.code, teamUuid);
}

void handleUnsubscribe(Client &clientData, ParsedInput &input)
{
    const std::string teamUuid = input.getArg<std::string>();
    if (input.fail() || input.hasRemainingArgs()) {
        std::cout << "Usage: /unsubscribe \"team_uuid\"" << std::endl;
        return;
    }
    if (!isUuidFormatValid(teamUuid)) {
        std::cout << "Invalid team UUID format." << std::endl;
        return;
    }

    myteams::PayloadReqTeamTarget payload {};
    copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), teamUuid);
    sendPacket(
        *clientData.socket,
        buildPacket(myteams::CMD_UNSUBSCRIBE, &payload, sizeof(payload)));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    (void)readServerPacket(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        (void)Printer::printUnsubscribed("", teamUuid);
        return;
    }
    if (responseHeader.code == myteams::ERR_FORBIDDEN) {
        std::cout << "You are not subscribed to this team." << std::endl;
        return;
    }
    handleCommonError(responseHeader.code, teamUuid);
}

void handleSubscribedList(Client &clientData, ParsedInput &input)
{
    std::string teamUuid;
    bool hasTeamFilter = false;

    if (input.hasRemainingArgs()) {
        teamUuid = input.getArg<std::string>();
        if (input.fail() || input.hasRemainingArgs()) {
            std::cout << "Usage: /subscribed [\"team_uuid\"]" << std::endl;
            return;
        }
        if (!isUuidFormatValid(teamUuid)) {
            std::cout << "Invalid team UUID format." << std::endl;
            return;
        }
        hasTeamFilter = true;
    }

    if (!hasTeamFilter) {
        sendPacket(*clientData.socket, buildPacket(myteams::CMD_SUBSCRIBED_LIST));
    } else {
        myteams::PayloadReqTeamTarget payload {};
        copyPaddedString(payload.team_uuid, sizeof(payload.team_uuid), teamUuid);
        sendPacket(
            *clientData.socket,
            buildPacket(myteams::CMD_SUBSCRIBED_LIST, &payload, sizeof(payload)));
    }

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    (void)readServerPacket(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
        (void)Printer::errorUnauthorized();
        return;
    }
    if (responseHeader.code == myteams::ERR_NOT_FOUND) {
        (void)Printer::errorUnknownTeam(teamUuid);
        return;
    }

    if (!hasTeamFilter) {
        if (responseHeader.code != myteams::RPL_TEAMS_LIST) {
            std::cout << "Unexpected response for /subscribed." << std::endl;
            return;
        }
        if ((responsePayload.size() % sizeof(myteams::PayloadRplTeam)) != 0) {
            std::cout << "Malformed team list payload received from server." << std::endl;
            return;
        }
        const std::size_t teamCount = responsePayload.size() / sizeof(myteams::PayloadRplTeam);
        for (std::size_t index = 0; index < teamCount; ++index) {
            myteams::PayloadRplTeam teamPayload {};
            std::memcpy(
                &teamPayload,
                responsePayload.data() + (index * sizeof(teamPayload)),
                sizeof(teamPayload));
            (void)Printer::printTeams(
                teamPayload.team_uuid,
                teamPayload.team_name,
                teamPayload.team_description);
        }
        return;
    }

    if (responseHeader.code != myteams::RPL_USERS_LIST) {
        std::cout << "Unexpected response for /subscribed \"team_uuid\"." << std::endl;
        return;
    }
    if ((responsePayload.size() % sizeof(myteams::PayloadRplUser)) != 0) {
        std::cout << "Malformed user list payload received from server." << std::endl;
        return;
    }
    const std::size_t userCount = responsePayload.size() / sizeof(myteams::PayloadRplUser);
    for (std::size_t index = 0; index < userCount; ++index) {
        myteams::PayloadRplUser userPayload {};
        std::memcpy(
            &userPayload,
            responsePayload.data() + (index * sizeof(userPayload)),
            sizeof(userPayload));
        (void)Printer::printUsers(
            userPayload.user_uuid,
            userPayload.user_name,
            static_cast<int>(userPayload.user_status));
    }
}

} // namespace client::commands
