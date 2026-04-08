#pragma once

#include <ctime>
#include <cstring>
#include <string>
#include <string_view>

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
            const std::string &uuid,
            const std::string &author_uuid,
            std::time_t created_at,
            const std::string &body)
            : created_at_(created_at)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(author_uuid_, author_uuid);
            copy_buffer(body_, body);
        }

        const std::string_view getUuid() const noexcept
        {
            return uuid_;
        }

        const std::string_view getAuthorUuid() const noexcept
        {
            return author_uuid_;
        }

        const std::time_t getCreatedAt() const noexcept
        {
            return created_at_;
        }

        const std::string_view getBody() const noexcept
        {
            return body_;
        }

        void setUuid(const std::string &uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setAuthorUuid(const std::string &author_uuid) noexcept
        {
            copy_buffer(author_uuid_, author_uuid);
        }

        void setCreatedAt(std::time_t created_at) noexcept
        {
            created_at_ = created_at;
        }

        void setBody(const std::string &body) noexcept
        {
            copy_buffer(body_, body);
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
        char author_uuid_[UUID_LENGTH] {};
        std::time_t created_at_ {};
        char body_[MAX_BODY_LENGTH] {};
    };
} // namespace myteams
