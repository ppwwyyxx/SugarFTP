import "string"
       "algorithm"
       "iostream"

import "Socket.h"
       "Common.h"
using namespace std

[public]
class Command
    cmd: string
    arg: string

    [const]
    string to_string() = cmd + " " + arg

[public]
class CmdHandler
    socket: SocketPtr

    CmdHandler() = @socket = nullptr
    CmdHandler(socket: SocketPtr) = @socket = socket

    Command recv()
        buf: char[1024]
        memset(buf, 0, sizeof(buf))
        size := 0
        loop
            throw(FTPExc("Unexpected EOF when reading command")) if not @socket->recv(buf + size, 1)
            break if buf[size] == '\n'
            size++
        size-- if size && buf[size - 1] == '\r'
        buf[size] = 0

        ret: Command
        ret.cmd.assign(buf)
        i := find_if(begin(buf), end(buf), (x : char) -> isspace(x)) - buf
        buf[i] = 0
        ret.cmd.assign(buf)
        ret.arg.assign(buf + i + 1)
        transform(begin(ret.cmd), end(ret.cmd), begin(ret.cmd), global::toupper)
        return ret

    void send(cmd: const string&, arg: const string& = "")
        buf := arg.empty() ? cmd : cmd + " " + arg
        buf.append("\r\n")
        @socket->send(buf.c_str(), buf.length())
