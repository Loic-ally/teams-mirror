#ifndef SERVER_MODELS_USER_USER_HPP
#define SERVER_MODELS_USER_USER_HPP

#include <cstdint>
#ifdef _WIN32
#pragma once
#endif

#include <cstring>
#include <string>
#include <string_view>

#include "common/limits.hpp"

namespace myteams
{
    /**
     * @brief Représente un utilisateur connecté au serveur.
     */
    class User
    {
    public:
        User() = default;

        User(const std::string &uuid, const std::string &name, bool is_logged_in = false)
            : is_logged_in_(is_logged_in)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(name_, name);
        }

        const std::string_view getUuid() const noexcept
        {
            return uuid_;
        }

        const std::string_view getName() const noexcept
        {
            return name_;
        }

        bool isLoggedIn() const noexcept
        {
            return is_logged_in_ >= 1;
        }

        void setUuid(const std::string &uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setName(const std::string &name) noexcept
        {
            copy_buffer(name_, name);
        }

        void resetLoggedIn() noexcept {
            is_logged_in_ = 0;
        }

        void addLoggedIn() noexcept
        {
            is_logged_in_ += 1;
        }

        void removeLoggedIn() noexcept
        {
            if (is_logged_in_ <= 0) {
                is_logged_in_ = 0;
            } else {
                is_logged_in_ -= 1;
            }
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
        std::int32_t is_logged_in_ = 0;
    };
} // namespace myteams

#endif // SERVER_MODELS_USER_USER_HPP
