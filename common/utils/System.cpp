#include "System.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace utils {

System::SystemException::SystemException(const std::string &str) : _err(str) {}

const char *System::SystemException::what() const noexcept {
    return _err.data();
}

std::size_t System::write(int fd, const std::string str) {
    auto res = ::write(fd, str.data(), str.length());
    if (res == -1) {
        throw System::SystemException("cannot write");
    }
    return res;
}

std::string System::read(int fd, std::size_t size) {
    std::string buf(size, '\0');
    auto res = ::read(fd, buf.data(), size);

    if (res == -1) {
        throw System::SystemException("cannot read");
    }
    buf.resize(res);
    return buf;
}

void System::getsockname(int fd, sockaddr &addr) {
    socklen_t size = sizeof(addr);
    if (::getsockname(fd, &addr, &size) == -1) {
        throw System::SystemException("cannot getsockname");
    }
}

int System::fork() {
    auto res = ::fork();
    if (res == -1) {
        throw System::SystemException("cannot fork");
    }
    return res;
}

int System::kill(int pid) {
    auto res = ::kill(pid, SIGTERM);
    if (res < 0) {
        throw System::SystemException("cannot kill");
    }
    return res;
}

int System::waitpid(int pid, bool block) {
    int stat = 0;
    int options = block ? 0 : WNOHANG;
    auto res = ::waitpid(pid, &stat, options);
    if (res < 0) {
        throw System::SystemException("cannot wait");
    }
    return res;
}

int System::inet_pton(int af, const char *ipStr, unsigned int &ipInt) {
    auto res = ::inet_pton(af, ipStr, &ipInt);
    if (res < 0) {
        throw System::SystemException("cannot inet_pton");
    }
    return res;
}

}