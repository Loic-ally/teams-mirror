#include "subscription_commands.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cstring>
#include <iostream>
#include <string>

namespace client::commands {

static void handleCommonError(std::uint16_t statusCode, const std::string &teamUuid)
{
    if (statusCode == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (statusCode == myteams::ERR_NOT_FOUND) {
        Printer::errorUnknownTeam(teamUuid);
        return;
    }
    if (statusCode == myteams::ERR_ALREADY_EXIST) {
        Printer::errorAlreadyExist();
        return;
    }
    if (statusCode == myteams::ERR_FORBIDDEN) {
        std::cout << "Operation forbidden in current context." << std::endl;
        return;
    }
    std::cout << "Server returned unexpected status: " << statusCode << std::endl;
}

static void printTeamEntry(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplTeam)) {
        std::cout << "Malformed team list payload received from server." << std::endl;
        return;
    }
    myteams::PayloadRplTeam teamPayload {};
    std::memcpy(&teamPayload, payload.data(), sizeof(teamPayload));
    Printer::printTeams(
        teamPayload.team_uuid,
        teamPayload.team_name,
        teamPayload.team_description);
}

static void printUserEntry(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplUser)) {
        std::cout << "Malformed user list payload received from server." << std::endl;
        return;
    }
    myteams::PayloadRplUser userPayload {};
    std::memcpy(&userPayload, payload.data(), sizeof(userPayload));
    Printer::printUsers(
        userPayload.user_uuid,
        userPayload.user_name,
        static_cast<int>(userPayload.user_status));
}

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
        buildPacket(myteams::CMD_SUBSCRIBE, payload));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        return;
    }
    if (responseHeader.code == myteams::RPL_USERS_LIST) {
        myteams::PayloadRplUser userPayload {};
        std::memcpy(&userPayload, responsePayload.data(), sizeof(userPayload));
        Printer::printSubscribed(userPayload.user_uuid, teamUuid);
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
        buildPacket(myteams::CMD_UNSUBSCRIBE, payload));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        return;
    }
    if (responseHeader.code == myteams::RPL_USERS_LIST) {
        myteams::PayloadRplUser userPayload {};
        std::memcpy(&userPayload, responsePayload.data(), sizeof(userPayload));
        Printer::printUnsubscribed(userPayload.user_uuid, teamUuid);
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
            buildPacket(myteams::CMD_SUBSCRIBED_LIST, payload));
    }

    while (true) {
        myteams::PacketHeader responseHeader {};
        std::string responsePayload;
        readServerReply(*clientData.socket, responseHeader, responsePayload);

        if (responseHeader.code == myteams::RPL_OK) {
            return;
        }
        if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
            Printer::errorUnauthorized();
            return;
        }
        if (responseHeader.code == myteams::ERR_NOT_FOUND) {
            Printer::errorUnknownTeam(teamUuid);
            return;
        }
        if (responseHeader.code == myteams::ERR_FORBIDDEN) {
            std::cout << "You are not allowed to access this team." << std::endl;
            return;
        }
        if (!hasTeamFilter && responseHeader.code == myteams::RPL_SUBSCRIBED_LIST) {
            printTeamEntry(responsePayload);
            continue;
        }
        if (hasTeamFilter && responseHeader.code == myteams::RPL_USERS_LIST) {
            printUserEntry(responsePayload);
            continue;
        }
        if (!hasTeamFilter) {
            std::cout << "Unexpected response for /subscribed." << std::endl;
            return;
        }
        std::cout << "Unexpected response for /subscribed \"team_uuid\"." << std::endl;
        return;
    }
}

} // namespace client::commands
