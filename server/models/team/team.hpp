#pragma once

#include <array>
#include <cstring>
#include <string_view>
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

        Team(std::string_view uuid, std::string_view name, std::string_view description)
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

        void addSubscribedUser(std::string_view user_uuid)
        {
            subscribed_user_uuids_.push_back(make_uuid(user_uuid));
        }

        void addChannel(const Channel &channel)
        {
            channels_.push_back(channel);
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

        static UserUuid make_uuid(std::string_view source) noexcept
        {
            UserUuid uuid {};
            const std::size_t copiedLength =
                source.size() < (uuid.size() - 1) ? source.size() : (uuid.size() - 1);
            std::memcpy(uuid.data(), source.data(), copiedLength);
            uuid[copiedLength] = '\0';
            return uuid;
        }

        char uuid_[UUID_LENGTH] {};
        char name_[MAX_NAME_LENGTH] {};
        char description_[MAX_DESCRIPTION_LENGTH] {};
        std::vector<UserUuid> subscribed_user_uuids_ {};
        std::vector<Channel> channels_ {};
    };
} // namespace myteams
