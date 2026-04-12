#include "packet_utils.hpp"
#include "common/protocol.hpp"
#include "display/printer.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

namespace client::commands {

static std::string readExact(const utils::Socket &socket, const std::size_t wantedSize)
{
    constexpr auto EMPTY_READ_BACKOFF = std::chrono::milliseconds(2);
    constexpr std::size_t MAX_CONSECUTIVE_EMPTY_READS = 2500;
    std::string buffer;
    std::size_t consecutiveEmptyReads = 0;
    buffer.reserve(wantedSize);
    while (buffer.size() < wantedSize) {
        const std::string chunk = socket.read(wantedSize - buffer.size());
        if (chunk.empty()) {
            ++consecutiveEmptyReads;
            if (consecutiveEmptyReads >= MAX_CONSECUTIVE_EMPTY_READS) {
                throw std::runtime_error("timed out waiting for server data");
            }
            std::this_thread::sleep_for(EMPTY_READ_BACKOFF);
            continue;
        }
        consecutiveEmptyReads = 0;
        buffer += chunk;
    }
    return buffer;
}

static void printUnexpectedPayload(const std::string_view message)
{
    std::cout << message << std::endl;
}

std::string buildPacket(const std::uint16_t code, const std::string_view payload)
{
    constexpr std::size_t MAX_PACKET_PAYLOAD_SIZE = std::numeric_limits<std::uint16_t>::max();
    if (payload.size() > MAX_PACKET_PAYLOAD_SIZE) {
        throw std::length_error("packet payload too large for protocol header");
    }
    const std::uint16_t payloadSize = static_cast<std::uint16_t>(payload.size());
    const myteams::PacketHeader header {code, payloadSize};
    std::string packet(sizeof(header) + payloadSize, '\0');

    std::memcpy(packet.data(), &header, sizeof(header));
    if (!payload.empty()) {
        std::memcpy(packet.data() + sizeof(header), payload.data(), payloadSize);
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

void readServerPacket(
    const utils::Socket &socket,
    myteams::PacketHeader &outHeader,
    std::string &outPayload)
{
    const std::string headerBuffer = readExact(socket, sizeof(myteams::PacketHeader));
    std::memcpy(&outHeader, headerBuffer.data(), sizeof(outHeader));

    outPayload.clear();
    if (outHeader.payload_size == 0) {
        return;
    }
    outPayload = readExact(socket, outHeader.payload_size);
}

bool handleAsyncEventPacket(const std::uint16_t code, const std::string &payload)
{
    if (code == myteams::EVT_LOGGED_IN) {
        if (payload.size() != sizeof(myteams::PayloadEvtUserConnection)) {
            printUnexpectedPayload("Malformed login event payload received from server.");
            return true;
        }
        myteams::PayloadEvtUserConnection eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventLoggedIn(eventPayload.user_uuid, eventPayload.user_name);
        return true;
    }
    if (code == myteams::EVT_LOGGED_OUT) {
        if (payload.size() != sizeof(myteams::PayloadEvtUserConnection)) {
            printUnexpectedPayload("Malformed logout event payload received from server.");
            return true;
        }
        myteams::PayloadEvtUserConnection eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventLoggedOut(eventPayload.user_uuid, eventPayload.user_name);
        return true;
    }
    if (code == myteams::EVT_PRIVATE_MSG_RCVD) {
        if (payload.size() != sizeof(myteams::PayloadEvtPrivateMsg)) {
            printUnexpectedPayload("Malformed private message event payload received from server.");
            return true;
        }
        myteams::PayloadEvtPrivateMsg eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventPrivateMessageReceived(eventPayload.sender_uuid, eventPayload.message_body);
        return true;
    }
    if (code == myteams::EVT_TEAM_CREATED) {
        if (payload.size() != sizeof(myteams::PayloadEvtTeamCreated)) {
            printUnexpectedPayload("Malformed team event payload received from server.");
            return true;
        }
        myteams::PayloadEvtTeamCreated eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventTeamCreated(
            eventPayload.team_uuid,
            eventPayload.team_name,
            eventPayload.team_description);
        return true;
    }
    if (code == myteams::EVT_CHANNEL_CREATED) {
        if (payload.size() != sizeof(myteams::PayloadEvtChannelCreated)) {
            printUnexpectedPayload("Malformed channel event payload received from server.");
            return true;
        }
        myteams::PayloadEvtChannelCreated eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventChannelCreated(
            eventPayload.channel_uuid,
            eventPayload.channel_name,
            eventPayload.channel_description);
        return true;
    }
    if (code == myteams::EVT_THREAD_CREATED) {
        if (payload.size() != sizeof(myteams::PayloadEvtThreadCreated)) {
            printUnexpectedPayload("Malformed thread event payload received from server.");
            return true;
        }
        myteams::PayloadEvtThreadCreated eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventThreadCreated(
            eventPayload.thread_uuid,
            eventPayload.user_uuid,
            static_cast<std::time_t>(eventPayload.thread_timestamp),
            eventPayload.thread_title,
            eventPayload.thread_body);
        return true;
    }
    if (code == myteams::EVT_REPLY_CREATED) {
        if (payload.size() != sizeof(myteams::PayloadEvtReplyCreated)) {
            printUnexpectedPayload("Malformed reply event payload received from server.");
            return true;
        }
        myteams::PayloadEvtReplyCreated eventPayload {};
        std::memcpy(&eventPayload, payload.data(), sizeof(eventPayload));
        Printer::eventThreadReplyReceived(
            eventPayload.team_uuid,
            eventPayload.thread_uuid,
            eventPayload.user_uuid,
            eventPayload.reply_body);
        return true;
    }

    return false;
}

void readServerReply(
    const utils::Socket &socket,
    myteams::PacketHeader &outHeader,
    std::string &outPayload)
{
    while(true) {
        readServerPacket(socket, outHeader, outPayload);
        if (!handleAsyncEventPacket(outHeader.code, outPayload)) {
            return;
        }
    }
}

bool isUuidFormatValid(const std::string_view uuid)
{
    if (uuid.size() != myteams::UUID_LENGTH - 1) {
        return false;
    }
    for (std::size_t index = 0; index < uuid.size(); ++index) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            if (uuid[index] != '-') {
                return false;
            }
            continue;
        }
        if (!std::isxdigit(static_cast<unsigned char>(uuid[index]))) {
            return false;
        }
    }
    return true;
}

}

