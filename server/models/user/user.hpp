#pragma once

#include <cstring>

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

        User(const char *uuid, const char *name, bool is_logged_in = false)
            : is_logged_in_(is_logged_in)
        {
            copy_buffer(uuid_, uuid);
            copy_buffer(name_, name);
        }

        const char *getUuid() const noexcept
        {
            return uuid_;
        }

        const char *getName() const noexcept
        {
            return name_;
        }

        bool isLoggedIn() const noexcept
        {
            return is_logged_in_;
        }

        void setUuid(const char *uuid) noexcept
        {
            copy_buffer(uuid_, uuid);
        }

        void setName(const char *name) noexcept
        {
            copy_buffer(name_, name);
        }

        void setLoggedIn(bool is_logged_in) noexcept
        {
            is_logged_in_ = is_logged_in;
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
        bool is_logged_in_ = false;
    };
} // namespace myteams
