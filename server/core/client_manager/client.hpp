#ifndef SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP
#define SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP

#ifdef _WIN32
#pragma once
#endif

#include "common/limits.hpp"

#include <cstring>
#include <string>
#include <string_view>

namespace server {

class Client {
    public:
        std::string incomingBuffer;
        std::string outgoingBuffer;

        void setContext(
            const std::string_view teamUuid,
            const std::string_view channelUuid,
            const std::string_view threadUuid) noexcept
        {
            copyUuid(_ctxTeamUuid, teamUuid);
            copyUuid(_ctxChannelUuid, channelUuid);
            copyUuid(_ctxThreadUuid, threadUuid);
        }

        std::string getContextTeamUuid() const noexcept
        {
            return readUuid(_ctxTeamUuid, sizeof(_ctxTeamUuid));
        }

        std::string getContextChannelUuid() const noexcept
        {
            return readUuid(_ctxChannelUuid, sizeof(_ctxChannelUuid));
        }

        std::string getContextThreadUuid() const noexcept
        {
            return readUuid(_ctxThreadUuid, sizeof(_ctxThreadUuid));
        }

    private:
        static void copyUuid(char *destination, const std::string_view source) noexcept
        {
            if (destination == nullptr) {
                return;
            }
            const std::size_t maxLen = myteams::UUID_LENGTH - 1;
            const std::size_t copiedLength = source.size() < maxLen ? source.size() : maxLen;
            std::memcpy(destination, source.data(), copiedLength);
            destination[copiedLength] = '\0';
        }

        static std::string readUuid(const char *source, const std::size_t size) noexcept
        {
            if (source == nullptr || size == 0) {
                return {};
            }
            const auto *end = static_cast<const char *>(std::memchr(source, '\0', size));
            if (end == nullptr) {
                return {};
            }
            return std::string(source, static_cast<std::size_t>(end - source));
        }

        char _ctxTeamUuid[myteams::UUID_LENGTH] {};
        char _ctxChannelUuid[myteams::UUID_LENGTH] {};
        char _ctxThreadUuid[myteams::UUID_LENGTH] {};
};

} // namespace server

#endif // SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP
