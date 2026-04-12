#ifndef SERVER_COMMANDS_COMMAND_UTILS_HPP
#define SERVER_COMMANDS_COMMAND_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
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

template <std::size_t BufferSize>
void copyPaddedString(char (&destination)[BufferSize], const std::string_view source)
{
    if constexpr (BufferSize > 0) {
        const std::size_t copiedLength = source.size() < (BufferSize - 1) ? source.size() : (BufferSize - 1);
        std::memcpy(destination, source.data(), copiedLength);
        destination[copiedLength] = '\0';
    }
}

template <std::size_t BufferSize>
void copyPaddedString(
    char (&destination)[BufferSize],
    const std::size_t destinationSize,
    const std::string_view source)
{
    (void)destinationSize;
    copyPaddedString(destination, source);
}

std::string buildPacket(std::uint16_t code, std::string_view payload = {});

template <typename PayloadType>
std::string buildPacket(const std::uint16_t code, const PayloadType &payload)
{
    std::string payloadBytes(sizeof(PayloadType), '\0');
    std::memcpy(payloadBytes.data(), &payload, payloadBytes.size());
    return buildPacket(code, std::string_view(payloadBytes));
}

template <typename PayloadType>
std::string buildPacket(const std::uint16_t code, const std::vector<PayloadType> &payloads)
{
    if (payloads.empty()) {
        return buildPacket(code);
    }
    constexpr std::size_t MAX_PACKET_PAYLOAD_SIZE = std::numeric_limits<std::uint16_t>::max();
    if (payloads.size() > (MAX_PACKET_PAYLOAD_SIZE / sizeof(PayloadType))) {
        return buildPacket(static_cast<std::uint16_t>(myteams::ERR_SERVER_INTERNAL));
    }
    std::string payloadBytes(payloads.size() * sizeof(PayloadType), '\0');
    std::memcpy(payloadBytes.data(), payloads.data(), payloadBytes.size());
    return buildPacket(code, std::string_view(payloadBytes));
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

bool extractFixedString(std::string_view rawData, std::string &outString);

template <std::size_t BufferSize>
bool extractFixedString(const char (&rawData)[BufferSize], std::string &outString)
{
    return extractFixedString(std::string_view(rawData, BufferSize), outString);
}

template <std::size_t BufferSize>
bool extractFixedString(const char (&rawData)[BufferSize], const std::size_t rawSize, std::string &outString)
{
    const std::size_t effectiveSize = rawSize < BufferSize ? rawSize : BufferSize;
    return extractFixedString(std::string_view(rawData, effectiveSize), outString);
}

std::string buildUserConnectionEventPacket(
    myteams::StatusCode eventCode,
    std::string_view userUuid,
    std::string_view userName);

} // namespace server::commands

#endif // SERVER_COMMANDS_COMMAND_UTILS_HPP

