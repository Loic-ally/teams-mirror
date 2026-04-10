#ifndef SERVER_COMMANDS_COMMAND_UTILS_HPP
#define SERVER_COMMANDS_COMMAND_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/protocol.hpp"
#include "server/commands/command_context.hpp"
#include "server/models/team/team.hpp"
#include "server/models/user/user.hpp"

namespace server::commands {

using UserRef = std::reference_wrapper<myteams::User>;
using TeamRef = std::reference_wrapper<myteams::Team>;
using ChannelRef = std::reference_wrapper<myteams::Channel>;
using ThreadRef = std::reference_wrapper<myteams::Thread>;

void copyPaddedString(char *destination, std::size_t size, std::string_view source);

std::string buildPacket(std::uint16_t code, std::string_view payload = {});

template <typename PayloadType>
std::string buildPacket(const std::uint16_t code, const PayloadType &payload)
{
    return buildPacket(
        code,
        std::string_view(reinterpret_cast<const char *>(&payload), sizeof(PayloadType)));
}

template <typename PayloadType>
std::string buildPacket(const std::uint16_t code, const std::vector<PayloadType> &payloads)
{
    if (payloads.empty()) {
        return buildPacket(code);
    }
    return buildPacket(
        code,
        std::string_view(
            reinterpret_cast<const char *>(payloads.data()),
            payloads.size() * sizeof(PayloadType)));
}

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

std::optional<UserRef> findUserByName(std::vector<myteams::User> &users, std::string_view userName);
std::optional<UserRef> findUserByUuid(std::vector<myteams::User> &users, std::string_view userUuid);
std::optional<TeamRef> findTeamByUuid(std::vector<myteams::Team> &teams, std::string_view teamUuid);
std::optional<ChannelRef> findChannelByUuid(myteams::Team &team, std::string_view channelUuid);
std::optional<ThreadRef> findThreadByUuid(myteams::Channel &channel, std::string_view threadUuid);
std::optional<UserRef> getAuthenticatedUser(CommandContext &context);

bool isUuidFormatValid(std::string_view uuid);
bool isContextCombinationValid(
    const std::string &teamUuid,
    const std::string &channelUuid,
    const std::string &threadUuid);

bool extractFixedString(const char *rawData, std::size_t rawSize, std::string &outString);

std::string buildUserConnectionEventPacket(
    myteams::StatusCode eventCode,
    std::string_view userUuid,
    std::string_view userName);

} // namespace server::commands

#endif // SERVER_COMMANDS_COMMAND_UTILS_HPP

