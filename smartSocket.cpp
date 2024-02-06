#include "smartSocket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <exception>
#include <stdexcept>

SmartSocket::SmartSocket(int socket)
{
    if (socket == -1) {
        throw std::bad_alloc();
    }
    this->socket = socket;
}

SmartSocket::~SmartSocket()
{
    close(this->socket);
}

int SmartSocket::fd() const {
    return socket;
}

int SmartSocket::setsockopt(int __level, int __optname, const void *__optval, socklen_t __optlen) const
{
    return ::setsockopt(socket, __level, __optname, __optval, __optlen);
}

int SmartSocket::bind(const sockaddr *__addr, socklen_t __len) const
{
    return ::bind(socket, __addr, __len);
}

int SmartSocket::listen(int __n) const
{
    return ::listen(socket, __n);
}

int SmartSocket::accept(sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len) const
{
    return ::accept(socket, __addr, __addr_len);
}

ssize_t SmartSocket::recv(void *__buf, size_t __n, int __flags) const
{
    return ::recv(socket, __buf, __n, __flags);
}

ssize_t SmartSocket::send(const void *__buf, size_t __n, int __flags) const
{
    return ::send(socket, __buf, __n, __flags);
}

int SmartSocket::connect(const sockaddr *__addr, socklen_t __len) const
{
    int status = (::connect(socket, __addr, __len));
    if (status) {
        throw std::runtime_error("backend connection failed");
    }
    return status;
}
