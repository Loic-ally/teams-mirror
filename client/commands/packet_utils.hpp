#ifndef CLIENT_COMMANDS_PACKET_UTILS_HPP
#define CLIENT_COMMANDS_PACKET_UTILS_HPP

#ifdef _WIN32
#pragma once
#endif

#include "Socket.hpp"
#include "common/protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace client::commands {

std::string buildPacket(
    std::uint16_t code,
    const void *payload = nullptr,
    std::uint16_t payloadSize = 0);

void sendPacket(const utils::Socket &socket, const std::string &packet);

void copyPaddedString(char *destination, std::size_t size, const std::string &source);

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
