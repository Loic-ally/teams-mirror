#include "System.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace utils {

System::SystemException::SystemException(const std::string &operation, int errorNumber)
    : std::runtime_error(operation), _errorNumber(errorNumber) {
}

int System::SystemException::errorNumber() const noexcept {
    return _errorNumber;
}

std::size_t System::write(int fd, const std::string &str) {
    auto res = ::write(fd, str.data(), str.length());
    if (res == -1) {
        throw System::SystemException("write() failed", errno);
    }
    return static_cast<std::size_t>(res);
}

std::string System::read(int fd, std::size_t size) {
    std::string buf(size, '\0');
    auto res = ::read(fd, buf.data(), size);

    if (res == -1) {
        throw System::SystemException("read() failed", errno);
    }
    buf.resize(res);
    return buf;
}

void System::getsockname(int fd, sockaddr &addr) {
    socklen_t size = sizeof(addr);
    if (::getsockname(fd, &addr, &size) == -1) {
        throw System::SystemException("getsockname() failed", errno);
    }
}

int System::fork() {
    auto res = ::fork();
    if (res == -1) {
        throw System::SystemException("fork() failed", errno);
    }
    return res;
}

int System::kill(int pid) {
    auto res = ::kill(pid, SIGTERM);
    if (res < 0) {
        throw System::SystemException("kill() failed", errno);
    }
    return res;
}

int System::waitpid(int pid, bool block) {
    int stat = 0;
    int options = block ? 0 : WNOHANG;
    auto res = ::waitpid(pid, &stat, options);
    if (res < 0) {
        throw System::SystemException("waitpid() failed", errno);
    }
    return res;
}

int System::inet_pton(int af, const char *ipStr, unsigned int &ipInt) {
    auto res = ::inet_pton(af, ipStr, &ipInt);
    if (res <= 0) {
        throw System::SystemException("inet_pton() failed", res == 0 ? EINVAL : errno);
    }
    return res;
}

}