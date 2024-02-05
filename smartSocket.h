#ifndef SMARTSOCKET_H
#define SMARTSOCKET_H

class SmartSocket {
public:
    SmartSocket(int socket);
    ~SmartSocket();
    int fd() const;

private:
    int socket;
};

#endif