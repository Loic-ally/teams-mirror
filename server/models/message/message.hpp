#pragma once

#include <ctime>
#include <cstring>

#include "common/limits.hpp"

namespace myteams
{
    /**
     * @brief Représente un message ou commentaire dans une discussion.
     */
    class Message
    {
    public:
        Message() = default;

        Message(
            const char *uuid,
            const char *author_uuid,
            std::time_t created_at,
            const char *body)
            : created_at_(created_at)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(author_uuid_, author_uuid);
            copy_buffer(body_, body);
        }

        const char *getUuid() const noexcept
        {
            return uuid_;
        }

        const char *getAuthorUuid() const noexcept
        {
            return author_uuid_;
        }

        std::time_t getCreatedAt() const noexcept
        {
            return created_at_;
        }

        const char *getBody() const noexcept
        {
            return body_;
        }

        void setUuid(const char *uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setAuthorUuid(const char *author_uuid) noexcept
        {
            copy_buffer(author_uuid_, author_uuid);
        }

        void setCreatedAt(std::time_t created_at) noexcept
        {
            created_at_ = created_at;
        }

        void setBody(const char *body) noexcept
        {
            copy_buffer(body_, body);
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
        char author_uuid_[UUID_LENGTH] {};
        std::time_t created_at_ {};
        char body_[MAX_BODY_LENGTH] {};
    };
} // namespace myteams
