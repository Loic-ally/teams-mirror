#ifndef LIMITS_HPP
#define LIMITS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <cstdint>

namespace myteams
{
    constexpr std::uint16_t MAX_NAME_LENGTH = 32;
    constexpr std::uint16_t MAX_DESCRIPTION_LENGTH = 255;
    constexpr std::uint16_t MAX_BODY_LENGTH = 512;
    constexpr std::uint16_t UUID_LENGTH = 37;
} //namespace myteams

#endif // LIMITS_HPP
