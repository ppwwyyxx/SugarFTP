#pragma once

class TCPSocket;
class ServerSocket;

#include "memory"
#include "errno.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "cstdio"
#include "netinet/in.h"
#include "netinet/tcp.h"
#include "cassert"
#include "netdb.h"
#include "string"
#include "string.h"
#include "sys/ioctl.h"
#include "sstream"
#include "Common.h"

using namespace std;

using SocketPtr = shared_ptr<TCPSocket>;
extern const int SOCKET_TIMEOUT;

class TCPSocket {
public:
    int fd = -1;

private:
    unsigned local_addr;
    int local_port;

public:
    TCPSocket();

    TCPSocket(int fd);

    ~TCPSocket();

    void close();

    void set_socket_fd(int fd);

    void send(const void *_buf, size_t size);

    size_t recv(void *buf, size_t max_size);

    static SocketPtr connect(const char *host, const char *service);

    void set_timeout();

    static string ip_addr(int h0, int h1, int h2, int h3, char sep = '.');

    int get_local_port() const;

    unsigned get_local_addr() const;
};

class ServerSocket: public TCPSocket {
public:
    ServerSocket(uint16_t port, int backlog = 5);

    SocketPtr accept();
};