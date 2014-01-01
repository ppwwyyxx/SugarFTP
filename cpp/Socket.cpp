#include "Socket.h"

const int SOCKET_TIMEOUT = 50;

TCPSocket::TCPSocket() {
    return;
}

TCPSocket::TCPSocket(int fd) {
    (this->set_socket_fd)(fd);
}

TCPSocket::~TCPSocket() {
    (this->close)();
}

void TCPSocket::close() {
    if ((this->fd) != -1) {
        if ((::close((this->fd)))) {
            print_debug("Failed to close fd " + to_string((this->fd)) + ", : " + strerror(errno));
        }
        (this->fd) = -1;
    }
}

void TCPSocket::set_socket_fd(int fd) {
    (this->fd) = fd;
    auto flag = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, ((const char*)&flag), sizeof(flag))) {
        throw(FTPExc(string("Failed to setsockopt: ") + strerror(errno)));
    }
    sockaddr_in local_addr;
    auto local_addr_ptr = ((sockaddr*)&local_addr);
    auto addrlen = ((socklen_t)sizeof(local_addr));
    memset(local_addr_ptr, 0, addrlen);
    getsockname(fd, local_addr_ptr, &addrlen);
    (this->local_addr) = ntohl(local_addr.sin_addr.s_addr);
    (this->local_port) = ntohs(local_addr.sin_port);
}

void TCPSocket::send(const void *_buf, size_t size) {
    auto buf = ((const char*)_buf);
    while (size) {
        auto s = ::send((this->fd), buf, size, 0);
        if (s < 0) {
            throw(FTPExc(string("socket: failed to write: ") + strerror(errno)));
        }
        size -= s;
        buf += s;
    }
}

size_t TCPSocket::recv(void *buf, size_t max_size) {
    auto s = ::recv((this->fd), buf, max_size, 0);
    if (s < 0) {
        throw(FTPExc(string("socket: failed to read: ") + strerror(errno)));
    }
    return s;
}

SocketPtr TCPSocket::connect(const char *host, const char *service) {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *result;
    auto s = getaddrinfo(host, service, &hints, &result);
    auto fd = -1;
    while (true) {
        auto rp = result;
        if (!rp) {
            break;
        }
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (!::connect(fd, rp->ai_addr, rp->ai_addrlen)) {
            break;
        }
        ::close(fd);
        rp = rp->ai_next;
    }
    freeaddrinfo(result);
    if (fd == -1) {
        throw(FTPExc(string("Failed to connect to (") + host + ", " + service + strerror(errno)));
    }
    return SocketPtr(new TCPSocket(fd));
}

void TCPSocket::set_timeout() {
    timeval timeout;
    timeout.tv_sec = SOCKET_TIMEOUT;
    setsockopt((this->fd), SOL_SOCKET, SO_RCVTIMEO, ((const void*)&timeout), sizeof(timeout));
    setsockopt((this->fd), SOL_SOCKET, SO_SNDTIMEO, ((const void*)&timeout), sizeof(timeout));
}

string TCPSocket::ip_addr(int h0, int h1, int h2, int h3, char sep) {
    string ret = to_string(h0);
    for (auto k : { h1, h2, h3 }) {
        ret += sep + to_string(k);
    }
    return ret;
}

int TCPSocket::get_local_port() const {
    return (this->local_port);
}

unsigned TCPSocket::get_local_addr() const {
    return (this->local_addr);
}

ServerSocket::ServerSocket(uint16_t port, int backlog) {
    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw(FTPExc(string("failed to create socket: ") + strerror(errno)));
    }
    sockaddr_in srv_addr;
    auto srv_addr_ptr = ((sockaddr*)&srv_addr);
    memset(srv_addr_ptr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(port);
    auto optval = 1;
    auto retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, ((const char*)&optval), sizeof(optval));
    assert(!retval);
    if (::bind(sockfd, srv_addr_ptr, sizeof(srv_addr)) < 0) {
        throw(FTPExc(string("failed to bind to ") + to_string(port) + ": " + strerror(errno)));
    }
    if (::listen(sockfd, backlog)) {
        throw(FTPExc(string("failed to listen : ") + strerror(errno)));
    }
    (this->set_socket_fd)(sockfd);
}

SocketPtr ServerSocket::accept() {
    sockaddr_in clt_addr;
    auto clt_addr_len = ((socklen_t)sizeof(clt_addr));
    int cltfd;
    while (true) {
        cltfd = ::accept((this->fd), ((sockaddr*)&clt_addr), &clt_addr_len);
        if (cltfd == -1) {
            if (errno != EINTR) {
                print_debug(string("bad client socket fd: ") + strerror(errno));
            }
            continue;
        }
        break;
    }
    char hostbuf[NI_MAXHOST];
    char servbuf[NI_MAXSERV];
    getnameinfo(((sockaddr*)&clt_addr), clt_addr_len, hostbuf, NI_MAXHOST, servbuf, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    return SocketPtr(new TCPSocket(cltfd));
}