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
    std::cout << "Server returned unexpected status: " << statusCode << std::endl;
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
        buildPacket(myteams::CMD_SUBSCRIBE, &payload, sizeof(payload)));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        Printer::printSubscribed("", teamUuid);
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
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_OK) {
        Printer::printUnsubscribed("", teamUuid);
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
    readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::ERR_UNAUTHORIZED) {
        Printer::errorUnauthorized();
        return;
    }
    if (responseHeader.code == myteams::ERR_NOT_FOUND) {
        Printer::errorUnknownTeam(teamUuid);
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
            Printer::printTeams(
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
        Printer::printUsers(
            userPayload.user_uuid,
            userPayload.user_name,
            static_cast<int>(userPayload.user_status));
    }
}

} // namespace client::commands
