#pragma once

#include "Socket.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace client::commands {

std::string buildPacket(
    std::uint16_t code,
    const void *payload = nullptr,
    std::uint16_t payloadSize = 0);

void sendPacket(const utils::Socket &socket, const std::string &packet);

void copyPaddedString(char *destination, std::size_t size, const std::string &source);

}

