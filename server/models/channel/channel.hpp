#pragma once

#include <cstring>
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

        Channel(std::string_view uuid, std::string_view name, std::string_view description)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(name_, name);
            copy_buffer(description_, description);
        }

        const char *getUuid() const noexcept
        {
            return uuid_;
        }

        const char *getName() const noexcept
        {
            return name_;
        }

        const char *getDescription() const noexcept
        {
            return description_;
        }

        const std::vector<Thread> &getThreads() const noexcept
        {
            return threads_;
        }

        void setUuid(std::string_view uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setName(std::string_view name) noexcept
        {
            copy_buffer(name_, name);
        }

        void setDescription(std::string_view description) noexcept
        {
            copy_buffer(description_, description);
        }

        void addThread(const Thread &thread)
        {
            threads_.push_back(thread);
        }

    private:

        template <std::size_t N>
        static void copy_buffer(char (&destination)[N], std::string_view source) noexcept
        {
            const std::size_t copiedLength =
                source.size() < (N - 1) ? source.size() : (N - 1);
            std::memcpy(destination, source.data(), copiedLength);
            destination[copiedLength] = '\0';
        }

        char uuid_[UUID_LENGTH] {};
        char name_[MAX_NAME_LENGTH] {};
        char description_[MAX_DESCRIPTION_LENGTH] {};
        std::vector<Thread> threads_ {};
    };
} // namespace myteams
