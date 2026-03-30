#ifndef SERVER_EXCEPTIONS_HPP
#define SERVER_EXCEPTIONS_HPP

#ifdef _WIN32
#pragma once
#endif

#include <stdexcept>
#include <string>
#include <system_error>

namespace server {

class ServerException : public std::runtime_error {
    public:
        explicit ServerException(const std::string &message)
            : std::runtime_error("[Server] " + message)
        {
        }
};

class ServerSocketException : public ServerException {
    public:
        explicit ServerSocketException(const std::string &message)
            : ServerException("[Socket] " + message)
        {
        }
};

class ServerSyscallException : public ServerSocketException {
    public:
        ServerSyscallException(const std::string &operation, int errorNumber)
            : ServerSocketException(operation + ": "
                + std::error_code(errorNumber, std::generic_category()).message())
        {
        }
};

class SocketCreationException : public ServerSyscallException {
    public:
        explicit SocketCreationException(int errorNumber)
            : ServerSyscallException("socket() failed", errorNumber)
        {
        }
};

class SocketOptionException : public ServerSyscallException {
    public:
        explicit SocketOptionException(int errorNumber)
            : ServerSyscallException("setsockopt(SO_REUSEADDR) failed", errorNumber)
        {
        }
};

class SocketBindException : public ServerSyscallException {
    public:
        explicit SocketBindException(int errorNumber)
            : ServerSyscallException("bind() failed", errorNumber)
        {
        }
};

class SocketListenException : public ServerSyscallException {
    public:
        explicit SocketListenException(int errorNumber)
            : ServerSyscallException("listen() failed", errorNumber)
        {
        }
};

class SocketPollException : public ServerSyscallException {
    public:
        explicit SocketPollException(int errorNumber)
            : ServerSyscallException("poll() failed", errorNumber)
        {
        }
};

class SocketAcceptException : public ServerSyscallException {
    public:
        explicit SocketAcceptException(int errorNumber)
            : ServerSyscallException("accept() failed", errorNumber)
        {
        }
};

class SocketReceiveException : public ServerSyscallException {
    public:
        explicit SocketReceiveException(int errorNumber)
            : ServerSyscallException("recv() failed", errorNumber)
        {
        }
};

class SocketSendException : public ServerSyscallException {
    public:
        explicit SocketSendException(int errorNumber)
            : ServerSyscallException("send() failed", errorNumber)
        {
        }
};

class SocketNotInitializedException : public ServerSocketException {
    public:
        SocketNotInitializedException()
            : ServerSocketException("listening socket is not initialized")
        {
        }
};

class SocketAlreadyInitializedException : public ServerSocketException {
    public:
        SocketAlreadyInitializedException()
            : ServerSocketException("listening socket is already initialized")
        {
        }
};

} // namespace server

#endif // SERVER_EXCEPTIONS_HPP
