#ifndef CLIENT_COMMANDS_PACKET_UTILS_HPP
#define CLIENT_COMMANDS_PACKET_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif

#include "Socket.hpp"
#include "common/protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace client::commands {

struct ReceivedPacket {
    myteams::PacketHeader header {};
    std::string payload;
};

std::string buildPacket(std::uint16_t code, std::string_view payload = {});

template <typename PayloadType>
std::string buildPacket(const std::uint16_t code, const PayloadType &payload)
{
    std::string payloadBytes(sizeof(PayloadType), '\0');
    std::memcpy(payloadBytes.data(), &payload, payloadBytes.size());
    return buildPacket(code, std::string_view(payloadBytes));
}

void sendPacket(const utils::Socket &socket, const std::string &packet);

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

void readServerPacket(
    const utils::Socket &socket,
    myteams::PacketHeader &outHeader,
    std::string &outPayload);

bool handleAsyncEventPacket(std::uint16_t code, const std::string &payload);

void readServerReply(
    const utils::Socket &socket,
    myteams::PacketHeader &outHeader,
    std::string &outPayload);

bool isUuidFormatValid(std::string_view uuid);

}

#endif // CLIENT_COMMANDS_PACKET_UTILS_HPP
