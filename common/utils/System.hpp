#ifndef COMMON_UTILS_SYSTEM_HPP
#define COMMON_UTILS_SYSTEM_HPP

#ifdef _WIN32
#pragma once
#endif

#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>

namespace utils {
class System {
  public:
    class SystemException : public std::runtime_error {
      public:
        SystemException(const std::string &operation, int errorNumber = 0);
        int errorNumber() const noexcept;

      private:
        int _errorNumber;
    };

    static std::size_t write(int fd, const std::string &str);
    static std::string read(int fd, std::size_t size);
    static void getsockname(int fd, sockaddr &addr);
    static int kill(int pid);
    static int waitpid(int pid, bool block = false);
    static int inet_pton(int af, std::string_view ipStr, unsigned int &ipInt);
};
}
#endif // COMMON_UTILS_SYSTEM_HPP
