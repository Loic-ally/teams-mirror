#pragma once

#include <exception>
#include <string>
#include <sys/socket.h>

namespace utils {
class System {
  public:
    class SystemException : std::exception {
        const std::string _err;

      public:
        SystemException(const std::string &str);
        const char *what() const noexcept override;
    };

    static std::size_t write(int fd, const std::string str);
    static std::string read(int fd, std::size_t size);
    static void getsockname(int fd, sockaddr &addr);
    static int fork();
    static int kill(int pid);
    static int waitpid(int pid, bool block = false);
    static int inet_pton(int af, const char *ipStr, unsigned int &ipInt);
};
}