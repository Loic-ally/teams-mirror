#pragma once

#include <memory>
#include <string>
#include <sys/socket.h>

namespace utils {

class Socket {
  private:
    int _fd = -1;

  public:
    // deleted
    Socket(const Socket &) = delete;
    Socket(const Socket &&) = delete;
    Socket &operator=(const Socket &&) = delete;
    Socket &operator=(const Socket &) = delete;

    // ctor
    Socket(int domain, int type, int protocol);
    Socket(int fd);
    ~Socket();

    // function
    void bind(const sockaddr &addr) const;
    void connect(const sockaddr &addr) const;
    void connect(const std::string &adress, int port) const;
    void listen(int backlog = 4096) const;
    std::pair<std::unique_ptr<Socket>, std::unique_ptr<struct sockaddr>>
    accept() const;
    std::int8_t poll(std::int8_t events,
                     std::size_t timeout = 0) const;
    std::size_t write(const std::string &str) const;
    std::string read(std::size_t size) const;

    // getter
    int getFd() const;
};

class SocketClosed : public std::exception {
  public:
    SocketClosed() = default;
    ~SocketClosed() = default;
    const char *what() const noexcept;
};

}
