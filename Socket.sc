import "memory"
       "errno.h"
       "unistd.h"
       "sys/types.h"
       "sys/socket.h"
       "cstdio"
       "netinet/in.h"
       "netinet/tcp.h"
       "cassert"
       "netdb.h"
       "string"
       "string.h"
       "sys/ioctl.h"
       "sstream"

import "Common.h"
using namespace std
typedef SocketPtr = shared_ptr<TCPSocket>

SOCKET_TIMEOUT : const int = 100

[public]
class TCPSocket
    [protected]
    fd : int = -1
    [private]
    peerinfo: string
    [private]
    local_addr: unsigned
    [private]
    local_port: int

    TCPSocket()
        return
    TCPSocket(fd: int) = @set_socket_fd(fd)
    ~TCPSocket() = @close()

    void close()
        if @fd != -1
            if (global::close(@fd))
                print_debug(
                    "Failed to close fd " + to_string(@fd) + ", : " + strerror(errno))
            @fd = -1

    void set_socket_fd(fd: int)
        @fd = fd

        // tcp no delay
        flag := 1
        if setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag))
            throw(FTPExc(string("Failed to setsockopt: ") + strerror(errno)))

        local_addr: sockaddr_in
        local_addr_ptr := (sockaddr*) &local_addr
        addrlen := (socklen_t)sizeof(local_addr)
        memset(local_addr_ptr, 0, addrlen)
        if getsockname(fd, local_addr_ptr, &addrlen)
            throw(FTPExc(string("getsockname: ") + strerror(errno)))

        @local_addr = ntohl(local_addr.sin_addr.s_addr)
        @local_port = ntohs(local_addr.sin_port)


    void send(_buf: const void*, size: size_t)
        throw(FTPExc("attempt to write to unbound socket")) if @fd < 0
        buf := (const char*)_buf
        while size
            s := global::send(@fd, buf, size, 0)
            throw(FTPExc(string("socket: failed to write: ") + strerror(errno))) if s < 0
            size -= s
            buf += s

    size_t recv(buf: void*, max_size: size_t)
        s := global::recv(@fd, buf, max_size, 0)
        throw(FTPExc(string("socket: failed to read: ") + strerror(errno))) if s < 0
        return s

    [static]
    SocketPtr connect(host: const char*, service: const char*)
        hints: addrinfo
        memset(&hints, 0, sizeof(hints))
        hints.ai_family = AF_INET
        hints.ai_socktype = SOCK_STREAM

        result: addrinfo*
        if s := getaddrinfo(host, service, &hints, &result)
            throw(FTPExc(string("Failed to getaddrinfo(") + host + ", " + service + gai_strerror(s)))

        fd := -1
        loop
            rp := result
            break if not rp
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)
            continue if fd == -1

            // success, break
            break if not global::connect(fd, rp->ai_addr, rp->ai_addrlen)
            global::close(fd)
            rp = rp->ai_next

        freeaddrinfo(result)
        if fd == -1
            throw(FTPExc(string("Failed to connect to (") + host + ", " + service + strerror(errno)))
        return TCPSocket::make_from_fd(fd)

    void enable_timeout()
        timeout: timeval
        timeout.tv_sec = SOCKET_TIMEOUT
        timeout.tv_usec = 0
        setsockopt(@fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout))
        setsockopt(@fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout))

    [static]
    string format_addr(addr: unsigned, sep: char='.') = TCPSocket::ip_addr(
        (addr >> 24) & 0xff
        (addr >> 16) & 0xff
        (addr >> 8) & 0xff
        addr & 0xff
        sep
        )

    [static]
    string ip_addr(h0: int, h1: int, h2: int, h3: int, sep: char='.')
        ret: string = to_string(h0)
        ret += sep + to_string(k) for k <- [h1, h2, h3]
        return ret

    [const]
    string get_peerinfo() = @peerinfo

    [const]
    int get_local_port() = @local_port

    [const]
    unsigned get_local_addr() = @local_addr

    [static]
    SocketPtr make_from_fd(fd: int, peerinfo: string = "")
        ret : SocketPtr(new TCPSocket(fd))
        ret->peerinfo = peerinfo
        return ret


[public]
class ServerSocket: TCPSocket
    ServerSocket(port: uint16, backlog: int = 5)
        sockfd := socket(AF_INET, SOCK_STREAM, 0)
        throw(FTPExc(string("failed to create socket: ") + strerror(errno))) if sockfd < 0
        srv_addr: sockaddr_in
        srv_addr_ptr := (sockaddr*)&srv_addr
        memset(srv_addr_ptr, 0, sizeof(srv_addr))
        srv_addr.sin_family = AF_INET
        srv_addr.sin_addr.s_addr = INADDR_ANY
        srv_addr.sin_port = htons(port)

        optval := 1
        retval := setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval))
        assert(not retval)

        if global::bind(sockfd, srv_addr_ptr, sizeof(srv_addr)) < 0
            throw(FTPExc(string("failed to bind to ") + to_string(port) + ": " + strerror(errno)))
        if global::listen(sockfd, backlog)
            throw(FTPExc(string("listen failed: ") + strerror(errno)))

        @set_socket_fd(sockfd)

    SocketPtr accept()
        clt_addr: sockaddr_in
        clt_addr_len := (socklen_t)sizeof(clt_addr)
        cltfd: int
        loop
            cltfd = global::accept(@fd, (sockaddr*)&clt_addr, &clt_addr_len)
            if cltfd == -1
                print_debug(string("bad client socket fd: ") + strerror(errno)) if errno != EINTR
                continue
            break

        hostbuf: char[NI_MAXHOST]
        servbuf: char[NI_MAXSERV]
        getnameinfo(
            (sockaddr*) &clt_addr
            clt_addr_len
            hostbuf, NI_MAXHOST
            servbuf, NI_MAXSERV
            NI_NUMERICHOST | NI_NUMERICSERV
        )
        return TCPSocket::make_from_fd(cltfd, string(hostbuf) + ":" + servbuf)

