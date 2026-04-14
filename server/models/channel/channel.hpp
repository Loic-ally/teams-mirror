#ifndef SERVER_MODELS_CHANNEL_CHANNEL_HPP
#define SERVER_MODELS_CHANNEL_CHANNEL_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "common/limits.hpp"
#include "server/models/thread/thread.hpp"

namespace myteams
{
    /**
     * @brief Représente un canal appartenant à une équipe.
     */
    class Channel
    {
    public:
        Channel() = default;

        Channel(const std::string &uuid, const std::string &name, const std::string &description)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(name_, name);
            copy_buffer(description_, description);
        }

        const std::string_view getUuid() const noexcept
        {
            return uuid_;
        }

        const std::string_view getName() const noexcept
        {
            return name_;
        }

        const std::string_view getDescription() const noexcept
        {
            return description_;
        }

        const std::vector<Thread> &getThreads() const noexcept
        {
            return threads_;
        }

        std::vector<Thread> &getThreads() noexcept
        {
            return threads_;
        }

        void setUuid(const std::string &uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setName(const std::string &name) noexcept
        {
            copy_buffer(name_, name);
        }

        void setDescription(const std::string &description) noexcept
        {
            copy_buffer(description_, description);
        }

        void addThread(const Thread &thread)
        {
            threads_.push_back(thread);
        }

    private:
        template <std::size_t N>
        static void copy_buffer(char (&destination)[N], const std::string &source) noexcept
        {
            const std::size_t copiedLength = source.size() < (N - 1) ? source.size() : (N - 1);
            std::memcpy(destination, source.data(), copiedLength);
            destination[copiedLength] = '\0';
        }

        char uuid_[UUID_LENGTH] {};
        char name_[MAX_NAME_LENGTH] {};
        char description_[MAX_DESCRIPTION_LENGTH] {};
        std::vector<Thread> threads_ {};
    };
} // namespace myteams

#endif // SERVER_MODELS_CHANNEL_CHANNEL_HPP
