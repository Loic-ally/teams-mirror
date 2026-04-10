#ifndef SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP
#define SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP

#ifdef _WIN32
#pragma once
#endif

#include "common/limits.hpp"

#include <algorithm>
#include <array>
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
            return readUuid(_ctxTeamUuid);
        }

        std::string getContextChannelUuid() const noexcept
        {
            return readUuid(_ctxChannelUuid);
        }

        std::string getContextThreadUuid() const noexcept
        {
            return readUuid(_ctxThreadUuid);
        }

    private:
        template <std::size_t ArraySize>
        static void copyUuid(
            std::array<char, ArraySize> &destination,
            const std::string_view source) noexcept
        {
            destination.fill('\0');
            if constexpr (ArraySize > 0) {
                const std::size_t maxLen = ArraySize - 1;
                const std::size_t copiedLength = source.size() < maxLen ? source.size() : maxLen;
                std::copy_n(source.data(), copiedLength, destination.data());
            }
        }

        template <std::size_t ArraySize>
        static std::string readUuid(const std::array<char, ArraySize> &source) noexcept
        {
            const auto end = std::find(source.begin(), source.end(), '\0');
            return std::string(source.begin(), end);
        }

        std::array<char, myteams::UUID_LENGTH> _ctxTeamUuid {};
        std::array<char, myteams::UUID_LENGTH> _ctxChannelUuid {};
        std::array<char, myteams::UUID_LENGTH> _ctxThreadUuid {};
};

} // namespace server

#endif // SERVER_CORE_CLIENT_MANAGER_CLIENT_HPP
