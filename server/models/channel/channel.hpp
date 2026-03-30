#pragma once

#include <cstring>
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

        Channel(const char *uuid, const char *name, const char *description)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(name_, name);
            copy_buffer(description_, description);
        }

        [[nodiscard]] const char *getUuid() const noexcept
        {
            return uuid_;
        }

        [[nodiscard]] const char *getName() const noexcept
        {
            return name_;
        }

        [[nodiscard]] const char *getDescription() const noexcept
        {
            return description_;
        }

        [[nodiscard]] const std::vector<Thread> &getThreads() const noexcept
        {
            return threads_;
        }

        void setUuid(const char *uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setName(const char *name) noexcept
        {
            copy_buffer(name_, name);
        }

        void setDescription(const char *description) noexcept
        {
            copy_buffer(description_, description);
        }

        void addThread(const Thread &thread)
        {
            threads_.push_back(thread);
        }

    private:
        template <std::size_t N>
        static void copy_buffer(char (&destination)[N], const char *source) noexcept
        {
            if (source == nullptr) {
                destination[0] = '\0';
                return;
            }
            std::strncpy(destination, source, N - 1);
            destination[N - 1] = '\0';
        }

        char uuid_[UUID_LENGTH] {};
        char name_[MAX_NAME_LENGTH] {};
        char description_[MAX_DESCRIPTION_LENGTH] {};
        std::vector<Thread> threads_ {};
    };
} // namespace myteams
