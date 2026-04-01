
#include "Socket.hpp"
#include "System.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>

namespace utils {

Socket::Socket(int domain, int type, int protocol)
    : _fd(socket(domain, type, protocol)) {
    if (_fd == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Socket didn't create");
    }
}

Socket::Socket(int fd) {
    if (fd < 0) {
        throw std::runtime_error("Socket is invalid");
    }
    struct stat statbuf{};
    if (fstat(fd, &statbuf) == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Socket is invalid");
    }
    if (!S_ISSOCK(statbuf.st_mode)) {
        throw std::runtime_error("Socket is invalid");
    }
    _fd = dup(fd);
    if (_fd == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Socket didn't create");
    }
}

Socket::~Socket() {
    close(_fd);
}

void Socket::bind(const sockaddr &addr) const {
    if (::bind(_fd, &addr, sizeof(addr)) == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Bind didn't work");
    }
}

void Socket::connect(const sockaddr &addr) const {
    if (::connect(_fd, &addr, sizeof(addr)) == 1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("connect didn't work");
    };
}

void Socket::connect(const std::string &adress, int port) const {
    unsigned int addr;
    System::inet_pton(AF_INET, adress.data(), addr);
    auto config = sockaddr_in{AF_INET, htons(port), in_addr{addr}, {}};
    connect(*reinterpret_cast<sockaddr *>(&config));
}

void Socket::listen(int backlog) const {
    if (::listen(_fd, backlog) == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Listen didn't work");
    }
}

std::pair<std::unique_ptr<Socket>, std::unique_ptr<struct sockaddr>>
Socket::accept() const {
    sockaddr_in addr{};
    socklen_t sockLen{sizeof(addr)};
    int fd = -1;

    fd = ::accept(_fd, reinterpret_cast<struct sockaddr *>(&addr), &sockLen);

    if (fd == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Accept didn't work");
    }

    auto res1 = std::make_unique<Socket>(fd);
    auto res2 = std::make_unique<struct sockaddr>(
        *reinterpret_cast<struct sockaddr *>(&addr));

    close(fd);
    return {std::move(res1), std::move(res2)};
}

std::int8_t Socket::poll(std::int8_t events, std::size_t timeout) const {
    struct pollfd config{
        _fd,
        events,
        0,
    };
    if (::poll(&config, 1, timeout) == -1) {
        std::cout << std::strerror(errno) << std::endl;
        throw std::runtime_error("Poll didn't work");
    }
    return config.revents;
}

std::size_t Socket::write(const std::string &str) const {
    auto pollRes = poll(POLLOUT);
    if (pollRes & POLLERR || pollRes & POLLHUP) {
        throw SocketClosed();
    }
    if (pollRes & POLLOUT) {
        return System::write(_fd, str);
    }
    return 0;
};

std::string Socket::read(std::size_t size) const {
    auto pollRes = poll(POLLIN);

    if (pollRes & POLLERR || pollRes & POLLHUP) {
        std::string read;
        if (pollRes & POLLIN) {
            read = System::read(_fd, size);
        }
        if (read.empty()) {
            throw SocketClosed();
        }
        return read;
    }
    if (pollRes & POLLIN) {
        return System::read(_fd, size);
    }
    return "";
};

int Socket::getFd() const {
    return _fd;
}

const char *SocketClosed::what() const noexcept {
    return "Socket closed unexceptly";
}

}
