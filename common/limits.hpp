#ifndef LIMITS_HPP
#define LIMITS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>

namespace myteams
{
    constexpr std::size_t MAX_NAME_LENGTH = 32;
    constexpr std::size_t MAX_DESCRIPTION_LENGTH = 255;
    constexpr std::size_t MAX_BODY_LENGTH = 512;
    constexpr std::size_t UUID_LENGTH = 37;
} // namespace myteams

#endif //LIMITS_HPP
