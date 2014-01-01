#include "Client.h"

FtpClient::FtpClient(SocketPtr socket) {
    (this->ctrl) = socket;
    (this->handler) = CmdHandler(socket);
    (this->response)();
    (this->cmd_send)("USER anonymous");
    (this->cmd_send)("TYPE I");
}

void FtpClient::quit() {
    (this->cmd_send)("QUIT");
}

Command FtpClient::cmd_send(const string &cmd) {
    (this->ctrl)->send((cmd + "\r\n").c_str(), cmd.length() + 2);
    return (this->response)();
}

Command FtpClient::response() {
    auto cmd = (this->handler).recv();
    if (cmd.cmd[0] != '1' && cmd.cmd[0] != '2') {
        print_debug("Bad response: " + cmd.to_string());
        throw(StopCommand());
    }
    return cmd;
}

SocketPtr FtpClient::open_data_conn() {
    auto cmd = cmd_send("PASV");
    int h0, h1, h2, h3, p0, p1;
    if (sscanf(cmd.arg.substr(cmd.arg.rfind(' ') + 1).c_str(), "(%d, %d, %d, %d, %d, %d)", &h0, &h1, &h2, &h3, &p0, &p1) != 6) {
        print_debug("Bad response for PASV: " + cmd.to_string());
        throw(StopCommand());
    }
    return TCPSocket::connect(TCPSocket::ip_addr(h0, h1, h2, h3).c_str(), to_string(p0 * 256 + p1).c_str());
}

void FtpClient::start() {
    while (true) {
        string cmd, arg;
        try {
            printf("SugarFTP> ");
            string s;
            if (!getline(cin, s)) {
                return;
            }
            istringstream iss { s };
            iss >> cmd >> arg;
            if (cmd == "help") {
                cout << "Available command: ls / cd / q(quit) / rm / put / get / help" << endl;
            } else if (cmd == "ls") {
                auto data_conn = (this->open_data_conn)();
                (this->cmd_send)("LIST " + arg);
                while (true) {
                    auto s = data_conn->recv((this->buf), sizeof((this->buf)));
                    if (s <= 0) {
                        break;
                    }
                    fwrite((this->buf), 1, s, stdout);
                    fflush(stdout);
                }
                (this->response)();
            } else if (cmd == "cd") {
                (this->cmd_send)("CWD " + arg);
            } else if (cmd == "rm") {
                (this->cmd_send)("DELE " + arg);
            } else if (cmd == "put") {
                auto fin = fopen(arg.c_str(), "rb");
                if (!fin) {
                    print_debug("Failed to open file" + arg);
                    throw(StopCommand());
                }
                class _t_finally_0 {
                public:
                    std::function<void()> finally;
                    ~_t_finally_0() { finally(); }
                } _t_finally_0 = { [&]() { fclose(fin); } };
                auto data_conn = (this->open_data_conn)();
                (this->cmd_send)(string("STOR ") + basename(strdupa(arg.c_str())));
                while (true) {
                    auto size = fread((this->buf), 1, sizeof((this->buf)), fin);
                    if (size <= 0) {
                        break;
                    }
                    data_conn->send((this->buf), size);
                }
                data_conn->close();
                (this->response)();
            } else if (cmd == "get") {
                auto fout = fopen(arg.c_str(), "wb");
                if (!fout) {
                    print_debug("Failed to open file" + arg);
                    throw(StopCommand());
                }
                class _t_finally_0 {
                public:
                    std::function<void()> finally;
                    ~_t_finally_0() { finally(); }
                } _t_finally_0 = { [&]() { fclose(fout); } };
                try {
                    auto data_conn = (this->open_data_conn)();
                    (this->cmd_send)("RETR " + arg);
                    while (true) {
                        auto size = data_conn->recv((this->buf), sizeof((this->buf)));
                        if (size <= 0) {
                            break;
                        }
                        fwrite((this->buf), 1, size, fout);
                    }
                    data_conn->close();
                    (this->response)();
                } catch (FTPExc &e) {
                    unlink(arg.c_str());
                    throw;
                }
            } else if (cmd == "q" || cmd == "quit") {
                return;
            } else {
                cout << "Command not supported, enter 'help' to see help" << endl;
            }
        } catch (StopCommand a) {
            continue;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <host> <port>" << endl;
        return -1;
    }
    try {
        FtpClient client { TCPSocket::connect(argv[1], argv[2]) };
        class _t_finally_0 {
        public:
            std::function<void()> finally;
            ~_t_finally_0() { finally(); }
        } _t_finally_0 = { [&]() { client.quit(); } };
        client.start();
    } catch (FTPExc e) {
        cerr << e.msg << endl;
    }
}