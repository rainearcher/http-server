#ifndef SMARTSOCKET_H
#define SMARTSOCKET_H
#include <sys/socket.h>

class SmartSocket {
public:
    SmartSocket(int socket);
    ~SmartSocket();
    int fd() const;
    
    int setsockopt(int __level, int __optname, const void *__optval, socklen_t __optlen) const;
    int bind(const sockaddr *__addr, socklen_t __len) const;
    int listen(int __n) const;
    int accept(sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len) const;
    ssize_t recv(void *__buf, size_t __n, int __flags) const;
    ssize_t send(const void *__buf, size_t __n, int __flags) const;
    int connect(const sockaddr *__addr, socklen_t __len) const;

private:
    int socket;
};

#endif