#ifndef SERVER_COMMANDS_COMMAND_UTILS_HPP
#define SERVER_COMMANDS_COMMAND_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "common/protocol.hpp"
#include "server/commands/command_context.hpp"
#include "server/models/user/user.hpp"

namespace server {
class ClientManager;
}

namespace server::commands {

void copyPaddedString(char *destination, std::size_t size, std::string_view source);

std::string buildPacket(std::uint16_t code, const void *payload = nullptr, std::uint16_t payloadSize = 0);

void queuePacket(ClientManager &clientManager, std::int32_t clientFd, const std::string &packet);
void queueStatus(ClientManager &clientManager, std::int32_t clientFd, myteams::StatusCode status);
void queueStatus(CommandContext &context, myteams::StatusCode status);

void broadcastPacket(
    ClientManager &clientManager,
    const ClientSockets &clientSockets,
    const std::string &packet,
    std::int32_t excludedClientFd = -1);
void broadcastPacket(CommandContext &context, const std::string &packet, std::int32_t excludedClientFd = -1);

std::string generateUuid();

myteams::User *findUserByName(std::vector<myteams::User> &users, std::string_view userName);
myteams::User *findUserByUuid(std::vector<myteams::User> &users, std::string_view userUuid);
myteams::Team *findTeamByUuid(std::vector<myteams::Team> &teams, std::string_view teamUuid);

bool isUuidFormatValid(std::string_view uuid);

bool extractFixedString(const char *rawData, std::size_t rawSize, std::string &outString);

std::string buildUserConnectionEventPacket(
    myteams::StatusCode eventCode,
    std::string_view userUuid,
    std::string_view userName);

} // namespace server::commands

#endif // SERVER_COMMANDS_COMMAND_UTILS_HPP

