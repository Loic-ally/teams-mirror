#ifndef SERVER_MODELS_TEAM_TEAM_HPP
#define SERVER_MODELS_TEAM_TEAM_HPP

#ifdef _WIN32
#pragma once
#endif

#include <array>
#include <algorithm>
#include <cstring>
#include <string>
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

        Team(const std::string &uuid, const std::string &name, const std::string &description)
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

        const std::vector<UserUuid> &getSubscribedUsers() const noexcept
        {
            return subscribed_user_uuids_;
        }

        const std::vector<Channel> &getChannels() const noexcept
        {
            return channels_;
        }

        std::vector<Channel> &getChannels() noexcept
        {
            return channels_;
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

        void addSubscribedUser(const std::string &user_uuid)
        {
            subscribed_user_uuids_.push_back(make_uuid(user_uuid));
        }

        bool isUserSubscribed(const std::string_view userUuid) const noexcept
        {
            const auto it = std::find_if(
                subscribed_user_uuids_.begin(),
                subscribed_user_uuids_.end(),
                [userUuid](const UserUuid &storedUuid) { return std::string_view(storedUuid.data()) == userUuid; });
            return it != subscribed_user_uuids_.end();
        }

        bool removeSubscribedUser(const std::string_view userUuid)
        {
            const auto previousSize = subscribed_user_uuids_.size();
            subscribed_user_uuids_.erase(
                std::remove_if(
                    subscribed_user_uuids_.begin(),
                    subscribed_user_uuids_.end(),
                    [userUuid](const UserUuid &storedUuid) {
                        return std::string_view(storedUuid.data()) == userUuid;
                    }),
                subscribed_user_uuids_.end());
            return subscribed_user_uuids_.size() != previousSize;
        }

        void addChannel(const Channel &channel)
        {
            channels_.push_back(channel);
        }

    private:
        template <std::size_t N>
        static void copy_buffer(char (&destination)[N], const std::string &source) noexcept
        {
            const std::size_t copiedLength = source.size() < (N - 1) ? source.size() : (N - 1);
            std::memcpy(destination, source.data(), copiedLength);
            destination[copiedLength] = '\0';
        }

        static UserUuid make_uuid(const std::string &source) noexcept
        {
            UserUuid uuid {};
            const std::size_t copiedLength = source.size() < (uuid.size() - 1) ? source.size() : (uuid.size() - 1);
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

#endif // SERVER_MODELS_TEAM_TEAM_HPP
