#pragma once

#include <array>
#include <cstring>
#include <vector>

#include "common/limits.hpp"
#include "server/models/channel/channel.hpp"

namespace myteams
{
    /**
     * @brief Représente une équipe et ses abonnements utilisateurs.
     */
    class Team
    {
    public:
        using UserUuid = std::array<char, UUID_LENGTH>;

        Team() = default;

        Team(const char *uuid, const char *name, const char *description)
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

        const std::vector<UserUuid> &getSubscribedUsers() const noexcept
        {
            return subscribed_user_uuids_;
        }

        const std::vector<Channel> &getChannels() const noexcept
        {
            return channels_;
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

        void addSubscribedUser(const char *user_uuid)
        {
            subscribed_user_uuids_.push_back(make_uuid(user_uuid));
        }

        void addChannel(const Channel &channel)
        {
            channels_.push_back(channel);
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

        static UserUuid make_uuid(const char *source) noexcept
        {
            UserUuid uuid {};

            if (source == nullptr) {
                uuid[0] = '\0';
                return uuid;
            }
            std::strncpy(uuid.data(), source, uuid.size() - 1);
            uuid[uuid.size() - 1] = '\0';
            return uuid;
        }

        char uuid_[UUID_LENGTH] {};
        char name_[MAX_NAME_LENGTH] {};
        char description_[MAX_DESCRIPTION_LENGTH] {};
        std::vector<UserUuid> subscribed_user_uuids_ {};
        std::vector<Channel> channels_ {};
    };
} // namespace myteams
