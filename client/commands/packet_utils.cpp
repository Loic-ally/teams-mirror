#include "packet_utils.hpp"
#include "common/protocol.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace client::commands {

std::string buildPacket(
    const std::uint16_t code,
    const void *payload,
    const std::uint16_t payloadSize)
{
    const myteams::PacketHeader header {code, payloadSize};
    std::string packet(sizeof(header) + payloadSize, '\0');

    std::memcpy(packet.data(), &header, sizeof(header));
    if (payload != nullptr && payloadSize > 0) {
        std::memcpy(packet.data() + sizeof(header), payload, payloadSize);
    }
    return packet;
}

void sendPacket(const utils::Socket &socket, const std::string &packet)
{
    std::size_t offset = 0;

    while (offset < packet.size()) {
        const std::size_t bytesWritten = socket.write(packet.substr(offset));
        if (bytesWritten == 0) {
            throw std::runtime_error("socket write returned 0");
        }
        offset += bytesWritten;
    }
}

void copyPaddedString(
    char *destination,
    const std::size_t size,
    const std::string &source)
{
    if (size == 0) {
        return;
    }
    const std::size_t copiedLength = std::min(source.size(), size - 1);
    std::memcpy(destination, source.data(), copiedLength);
    destination[copiedLength] = '\0';
}

}

