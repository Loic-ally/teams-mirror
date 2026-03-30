#pragma once

#include <ctime>
#include <cstring>
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
            const char *uuid,
            const char *author_uuid,
            std::time_t created_at,
            const char *title,
            const char *body)
            : created_at_(created_at)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(author_uuid_, author_uuid);
            copy_buffer(title_, title);
            copy_buffer(body_, body);
        }

        [[nodiscard]] const char *getUuid() const noexcept
        {
            return uuid_;
        }

        [[nodiscard]] const char *getAuthorUuid() const noexcept
        {
            return author_uuid_;
        }

        [[nodiscard]] std::time_t getCreatedAt() const noexcept
        {
            return created_at_;
        }

        [[nodiscard]] const char *getTitle() const noexcept
        {
            return title_;
        }

        [[nodiscard]] const char *getBody() const noexcept
        {
            return body_;
        }

        [[nodiscard]] const std::vector<Message> &getReplies() const noexcept
        {
            return replies_;
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

        void setTitle(const char *title) noexcept
        {
            copy_buffer(title_, title);
        }

        void setBody(const char *body) noexcept
        {
            copy_buffer(body_, body);
        }

        void addReply(const Message &message)
        {
            replies_.push_back(message);
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
        char title_[MAX_NAME_LENGTH] {};
        char body_[MAX_BODY_LENGTH] {};
        std::vector<Message> replies_ {};
    };
} // namespace myteams
