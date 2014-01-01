#include "Server.h"

FtpServer *server;



void FtpServer::chroot(const string &dir) {
    auto p = realpath(dir.c_str(), nullptr);
    if (!p) {
        throw(FTPExc("Failed to chroot to " + dir + " : " + strerror(errno)));
    }
    (this->rootdir).assign(p);
    free(p);
    if ((this->rootdir).back() != '/') {
        (this->rootdir).append("/");
    }
}

void FtpServer::set_port(int port) {
    (this->port) = port;
}

void FtpServer::work(FtpSession *session) {
    class _t_finally_0 {
    public:
        std::function<void()> finally;
        ~_t_finally_0() { finally(); }
    } _t_finally_0 = { [&]() { delete(session); } };
    try {
        session->run();
        print_debug("Session exited");
    } catch (FTPExc e) {
        print_debug("Exception Happened: " + e.msg);
    }
}

void FtpServer::start() {
    ServerSocket socket { (this->port) };
    print_debug("Server listening on : " + to_string(socket.get_local_port()));
    auto id = 0;
    while (true) {
        auto client = new FtpSession(socket.accept(), id++);
        auto job = thread(FtpServer::work, client);
        job.detach();
    }
}

FtpSession::FtpSession(const SocketPtr &socket, int clt_id) {
    (this->handler) = CmdHandler(socket);
    (this->ctrl) = socket;
    (this->clt_id) = clt_id;
    (this->ctrl)->set_timeout();
    print_debug("new client.");
}

void FtpSession::close_data_conn(SocketPtr socket, const string &msg) {
    socket->close();
    (this->handler).send("226", msg);
}

SocketPtr FtpSession::get_data_conn(const string &msg) {
    if (!(this->passive)) {
        (this->handler).send("425", "use PASV first");
        throw(StopCommand());
    }
    auto ret = (this->srv_socket)->accept();
    (this->srv_socket).reset();
    (this->passive) = false;
    (this->handler).send("125", msg);
    ret->set_timeout();
    return ret;
}

string FtpSession::safe_path(const string &fpath, bool allow_nonexist) {
    string &rootdir = server->rootdir;
    string dirname, basename, query_path;
    if (allow_nonexist) {
        basename = fpath;
        auto i = fpath.rfind('/');
        if (i != string::npos) {
            dirname.assign(fpath.c_str(), i);
            basename.assign(fpath.c_str() + i);
        }
        query_path = dirname;
    } else {
        query_path = fpath;
    }
    char *retc;
    if (query_path[0] == '/') {
        retc = realpath((rootdir + query_path).c_str(), nullptr);
    } else {
        retc = realpath((rootdir + working_dir + "/" + query_path).c_str(), nullptr);
    }
    if (!retc) {
        (this->handler).send("550", "bad file path");
        throw(StopCommand());
    }
    string ret { retc };
    free(retc);
    if (ret.substr(0, rootdir.length()) != rootdir && ret != rootdir.substr(0, rootdir.length() - 1)) {
        (this->handler).send("550", "bad file path");
        throw(StopCommand());
    }
    if (allow_nonexist) {
        if (ret.back() != '/') {
            ret.append("/");
        }
        ret.append(basename);
    }
    return ret;
}

void FtpSession::run() {
    try {
        (this->handler).send("220", "Sugar Ftp");
        while (true) {
            try {
                execute();
            } catch (StopCommand e) {
                continue;
            }
        }
    } catch (Quit e) {
        return;
    }
}

void FtpSession::execute() {
    (this->cur_cmd) = (this->handler).recv();
    print_debug("Client : " + (this->cur_cmd).to_string());
    auto cmd = (this->cur_cmd).cmd;
    if (cmd == "USER") {
        (this->handler).send("230", "welcome!");
    } else if (cmd == "TYPE") {
        (this->handler).send("200", "binary");
    } else if (cmd == "QUIT") {
        (this->handler).send("221", "Goodbye");
        throw(Quit());
    } else if (cmd == "FEAT") {
        (this->handler).send("211", "");
    } else if (cmd == "ALLO") {
        (this->handler).send("202", "ALLO!");
    } else if (cmd == "PWD") {
        (this->handler).send("257", "\"" + (this->working_dir) + "\"");
    } else if (cmd == "PASV") {
        (this->passive) = true;
        auto addr = (this->ctrl)->get_local_addr();
        shared_ptr<ServerSocket> tmp { new ServerSocket(0) };
        (this->srv_socket) = tmp;
        (this->srv_socket)->set_timeout();
        auto port = (this->srv_socket)->get_local_port();
        auto str_addr = TCPSocket::ip_addr((addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff, ',');
        (this->handler).send("227", "Entering Passive Mode (" + str_addr + "," + to_string(port >> 8) + "," + to_string(port & 0xff) + ").");
    } else if (cmd == "LIST") {
        auto path = (this->cur_cmd).arg;
        if (path[0] == '-') {
            path.erase(begin(path), find_if(begin(path), end(path), ([&](char x) {
                return (isspace(x));
            })));
        }
        path = (this->safe_path)(path);
        auto ls = "[[ -f \"" + path + "\" ]] && ls -al " + path + " || ls -al " + path + " | tail -n +2";
        auto data_conn = (this->get_data_conn)("start listing");
        auto ret = exec(ls);
        data_conn->send(ret.c_str(), ret.length());
        (this->close_data_conn)(data_conn, "finished litsing");
    } else if (cmd == "CWD") {
        auto new_dir = (this->safe_path)((this->cur_cmd).arg);
        struct stat t_stat;
        if (::stat(new_dir.c_str(), &t_stat) || !S_ISDIR(t_stat.st_mode)) {
            (this->handler).send("550", "failed to chdir");
            return;
        }
        (this->working_dir) = new_dir;
        (this->working_dir).erase(0, server->rootdir.length() - 1);
        (this->handler).send("250", "working dir changed to " + (this->working_dir));
    } else if (cmd == "RETR") {
        auto realpath = (this->safe_path)((this->cur_cmd).arg);
        FILE *fin = fopen(realpath.c_str(), "rb");
        if (!fin) {
            (this->handler).send("550", "failed to open file");
            return;
        }
        class _t_finally_0 {
        public:
            std::function<void()> finally;
            ~_t_finally_0() { finally(); }
        } _t_finally_0 = { [&]() { fclose(fin); } };
        auto data_conn = (this->get_data_conn)("going to transfer " + (this->cur_cmd).arg);
        while (true) {
            auto size = fread((this->buf), 1, sizeof((this->buf)), fin);
            if (size <= 0) {
                break;
            }
            data_conn->send((this->buf), size);
        }
        (this->close_data_conn)(data_conn, "transfer completed");
    } else if (cmd == "STOR") {
        auto realpath = (this->safe_path)((this->cur_cmd).arg, 1);
        FILE *fout = fopen(realpath.c_str(), "wb");
        if (!fout) {
            (this->handler).send("553", "failed to open file for write");
            return;
        }
        class _t_finally_0 {
        public:
            std::function<void()> finally;
            ~_t_finally_0() { finally(); }
        } _t_finally_0 = { [&]() { fclose(fout); } };
        auto data_conn = (this->get_data_conn)("OK to transfer");
        auto totsize = 0;
        while (true) {
            auto size = data_conn->recv((this->buf), sizeof((this->buf)));
            if (size <= 0) {
                break;
            }
            totsize += size;
            fwrite((this->buf), 1, size, fout);
        }
        print_debug("client : upload file " + realpath + " size: " + to_string(totsize));
        (this->close_data_conn)(data_conn, "transfer completed");
    } else if (cmd == "SIZE") {
        auto realpath = (this->safe_path)((this->cur_cmd).arg);
        struct stat stat;
        if (::stat(realpath.c_str(), &stat)) {
            (this->handler).send("550", "failed to get file size");
        }
        (this->handler).send("213", to_string(stat.st_size));
    } else {
        (this->handler).send("502", "unimplemented command");
    }
}

int main(int argc, char **argv) {
    FtpServer s;
    server = &s;
    if (argc == 1) {
        cerr << "Usage: " << argv[0] << " <root dir> [port = 8888]" << endl;
        return -1;
    }
    try {
        if (argc >= 2) {
            server->chroot(argv[1]);
        }
        if (argc >= 3) {
            server->set_port(atoi(argv[2]));
        } else {
            server->set_port(8888);
        }
        server->start();
    } catch (FTPExc e) {
        cout << e.msg << endl;
    }
}