#include "smartSocket.h"
#include <sys/socket.h>
#include <unistd.h>

SmartSocket::SmartSocket(int socket)
{
    this->socket = socket;
}

SmartSocket::~SmartSocket()
{
    close(this->socket);
}

int SmartSocket::fd() const {
    return socket;
}