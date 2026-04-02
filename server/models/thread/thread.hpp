#pragma once

#include <ctime>
#include <cstring>
#include <string_view>
#include <vector>

#include "common/limits.hpp"
#include "server/models/message/message.hpp"

namespace myteams
{
    /**
     * @brief Représente une discussion créée dans un canal.
     */
    class Thread
    {
    public:
        Thread() = default;

        Thread(
            std::string_view uuid,
            std::string_view author_uuid,
            std::time_t created_at,
            std::string_view title,
            std::string_view body)
            : created_at_(created_at)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(author_uuid_, author_uuid);
            copy_buffer(title_, title);
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

        const char *getTitle() const noexcept
        {
            return title_;
        }

        const char *getBody() const noexcept
        {
            return body_;
        }

        const std::vector<Message> &getReplies() const noexcept
        {
            return replies_;
        }

        void setUuid(std::string_view uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setAuthorUuid(std::string_view author_uuid) noexcept
        {
            copy_buffer(author_uuid_, author_uuid);
        }

        void setCreatedAt(std::time_t created_at) noexcept
        {
            created_at_ = created_at;
        }

        void setTitle(std::string_view title) noexcept
        {
            copy_buffer(title_, title);
        }

        void setBody(std::string_view body) noexcept
        {
            copy_buffer(body_, body);
        }

        void addReply(const Message &message)
        {
            replies_.push_back(message);
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
        char author_uuid_[UUID_LENGTH] {};
        std::time_t created_at_ {};
        char title_[MAX_NAME_LENGTH] {};
        char body_[MAX_BODY_LENGTH] {};
        std::vector<Message> replies_ {};
    };
} // namespace myteams
