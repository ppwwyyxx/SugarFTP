#include "Command.h"

string Command::to_string() const {
    return cmd + " " + arg;
}

CmdHandler::CmdHandler() {
    (this->socket) = nullptr;
}

CmdHandler::CmdHandler(SocketPtr socket) {
    (this->socket) = socket;
}

Command CmdHandler::recv() {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    auto size = 0;
    while (true) {
        if (!(this->socket)->recv(buf + size, 1)) {
            throw(FTPExc("Unexpected EOF when reading command"));
        }
        if (buf[size] == '\n') {
            break;
        }
        size++;
    }
    if (size && buf[size - 1] == '\r') {
        size--;
    }
    buf[size] = 0;
    Command ret;
    ret.cmd.assign(buf);
    auto i = find_if(begin(buf), end(buf), ([&](char x) {
        return isspace(x);
    })) - buf;
    buf[i] = 0;
    ret.cmd.assign(buf);
    ret.arg.assign(buf + i + 1);
    transform(begin(ret.cmd), end(ret.cmd), begin(ret.cmd), ::toupper);
    return ret;
}

void CmdHandler::send(const string &cmd, const string &arg) {
    auto buf = arg.empty() ? cmd : cmd + " " + arg;
    buf.append("\r\n");
    (this->socket)->send(buf.c_str(), buf.length());
}