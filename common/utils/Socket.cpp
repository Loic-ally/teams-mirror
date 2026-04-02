
#include "Socket.hpp"
#include "System.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace utils {

SocketException::SocketException(const std::string &message, int errorNumber) noexcept
    : std::runtime_error(message), _errorNumber(errorNumber) {
}

int SocketException::errorNumber() const noexcept {
    return _errorNumber;
}

Socket::Socket(int domain, int type, int protocol)
    : _fd(socket(domain, type, protocol)) {
    if (_fd == -1) {
        throw SocketException("socket() failed", errno);
    }
}

Socket::Socket(int fd) {
    if (fd < 0) {
        throw SocketException("socket fd is invalid", EBADF);
    }
    struct stat statbuf{};
    if (fstat(fd, &statbuf) == -1) {
        throw SocketException("fstat() failed for socket fd", errno);
    }
    if (!S_ISSOCK(statbuf.st_mode)) {
        throw SocketException("fd is not a socket", ENOTSOCK);
    }
    _fd = dup(fd);
    if (_fd == -1) {
        throw SocketException("dup() failed for socket fd", errno);
    }
}

Socket::~Socket() {
    if (_fd >= 0) {
        (void)::close(_fd);
    }
}

void Socket::setReuseAddress() const {
    const int reuseAddr = 1;
    if (::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) == -1) {
        throw SocketException("setsockopt(SO_REUSEADDR) failed", errno);
    }
}

void Socket::bind(const sockaddr &addr) const {
    if (::bind(_fd, &addr, sizeof(addr)) == -1) {
        throw SocketException("bind() failed", errno);
    }
}

void Socket::connect(const sockaddr &addr) const {
    if (::connect(_fd, &addr, sizeof(addr)) == -1) {
        throw SocketException("connect() failed", errno);
    };
}

void Socket::connect(const std::string &adress, int port) const {
    unsigned int addr;
    System::inet_pton(AF_INET, adress.data(), addr);
    sockaddr_in config{};
    config.sin_family = AF_INET;
    config.sin_port = htons(port);
    config.sin_addr = in_addr{addr};
    connect(*reinterpret_cast<sockaddr *>(&config));
}

void Socket::listen(int backlog) const {
    if (::listen(_fd, backlog) == -1) {
        throw SocketException("listen() failed", errno);
    }
}

std::pair<std::unique_ptr<Socket>, std::unique_ptr<struct sockaddr>>
Socket::accept() const {
    sockaddr_in addr{};
    socklen_t sockLen{sizeof(addr)};
    int fd = -1;

    fd = ::accept(_fd, reinterpret_cast<struct sockaddr *>(&addr), &sockLen);

    if (fd == -1) {
        throw SocketException("accept() failed", errno);
    }

    auto res1 = std::make_unique<Socket>(fd);
    auto res2 = std::make_unique<struct sockaddr>(
        *reinterpret_cast<struct sockaddr *>(&addr));

    (void)::close(fd);
    return {std::move(res1), std::move(res2)};
}

std::int64_t Socket::send(const void *buffer, std::size_t size) const {
    ssize_t sendResult = -1;
    do {
        sendResult = ::send(_fd, buffer, size, 0);
    } while (sendResult < 0 && errno == EINTR);
    if (sendResult < 0) {
        throw SocketException("send() failed", errno);
    }
    return static_cast<std::int64_t>(sendResult);
}

std::int64_t Socket::recv(void *buffer, std::size_t size) const {
    ssize_t recvResult = -1;
    do {
        recvResult = ::recv(_fd, buffer, size, 0);
    } while (recvResult < 0 && errno == EINTR);
    if (recvResult < 0) {
        throw SocketException("recv() failed", errno);
    }
    return static_cast<std::int64_t>(recvResult);
}

std::int8_t Socket::poll(std::int8_t events, std::size_t timeout) const {
    struct pollfd config{
        _fd,
        events,
        0,
    };
    if (::poll(&config, 1, timeout) == -1) {
        throw SocketException("poll() failed", errno);
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

SocketClosed::SocketClosed() noexcept : _message("socket closed unexpectedly") {
}

const char *SocketClosed::what() const noexcept {
    return _message;
}

}
