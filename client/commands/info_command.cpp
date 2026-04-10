#include "info_command.hpp"
#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "core/client.hpp"
#include "display/printer.hpp"
#include "parser/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

namespace client::commands {
namespace {

void handleInfoError(const std::uint16_t code)
{
    if (code == myteams::ERR_UNAUTHORIZED) {
        (void)Printer::errorUnauthorized();
        return;
    }
    if (code == myteams::ERR_NOT_FOUND) {
        std::cout << "Requested context entity does not exist." << std::endl;
        return;
    }
    if (code == myteams::ERR_BAD_REQUEST) {
        std::cout << "Invalid context for /info." << std::endl;
        return;
    }

    std::cout << "Server returned unexpected status: " << code << std::endl;
}

bool parseUserInfoPayload(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplUser)) {
        std::cout << "Malformed user info payload received from server." << std::endl;
        return false;
    }

    myteams::PayloadRplUser userPayload {};
    std::memcpy(&userPayload, payload.data(), sizeof(userPayload));
    (void)Printer::printUser(
        userPayload.user_uuid,
        userPayload.user_name,
        static_cast<int>(userPayload.user_status));
    return true;
}

bool parseTeamInfoPayload(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplTeam)) {
        std::cout << "Malformed team info payload received from server." << std::endl;
        return false;
    }

    myteams::PayloadRplTeam teamPayload {};
    std::memcpy(&teamPayload, payload.data(), sizeof(teamPayload));
    (void)Printer::printTeam(
        teamPayload.team_uuid,
        teamPayload.team_name,
        teamPayload.team_description);
    return true;
}

bool parseChannelInfoPayload(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplChannel)) {
        std::cout << "Malformed channel info payload received from server." << std::endl;
        return false;
    }

    myteams::PayloadRplChannel channelPayload {};
    std::memcpy(&channelPayload, payload.data(), sizeof(channelPayload));
    (void)Printer::printChannel(
        channelPayload.channel_uuid,
        channelPayload.channel_name,
        channelPayload.channel_description);
    return true;
}

bool parseThreadInfoPayload(const std::string &payload)
{
    if (payload.size() != sizeof(myteams::PayloadRplThread)) {
        std::cout << "Malformed thread info payload received from server." << std::endl;
        return false;
    }

    myteams::PayloadRplThread threadPayload {};
    std::memcpy(&threadPayload, payload.data(), sizeof(threadPayload));
    (void)Printer::printThread(
        threadPayload.thread_uuid,
        threadPayload.user_uuid,
        static_cast<std::time_t>(threadPayload.thread_timestamp),
        threadPayload.thread_title,
        threadPayload.thread_body);
    return true;
}

} // namespace

void handleInfo(Client &clientData, ParsedInput &input)
{
    if (input.hasRemainingArgs()) {
        std::cout << "Usage: /info" << std::endl;
        return;
    }

    sendPacket(*clientData.socket, buildPacket(myteams::CMD_INFO));

    myteams::PacketHeader responseHeader {};
    std::string responsePayload;
    (void)readServerReply(*clientData.socket, responseHeader, responsePayload);

    if (responseHeader.code == myteams::RPL_USER_INFO) {
        (void)parseUserInfoPayload(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_TEAMS_LIST) {
        (void)parseTeamInfoPayload(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_CHANNELS_LIST) {
        (void)parseChannelInfoPayload(responsePayload);
        return;
    }
    if (responseHeader.code == myteams::RPL_THREADS_LIST) {
        (void)parseThreadInfoPayload(responsePayload);
        return;
    }

    handleInfoError(responseHeader.code);
}

} // namespace client::commands
