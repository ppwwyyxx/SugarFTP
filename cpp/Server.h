#pragma once

class StopCommand;
class Quit;
class FtpServer;
class FtpSession;

#include "iostream"
#include "thread"
#include "cstdlib"
#include "sys/stat.h"
#include "unistd.h"
#include "cstring"
#include "Common.h"
#include "Socket.h"
#include "Command.h"

using namespace std;

extern FtpServer *server;

class StopCommand {
};

class Quit {
};

class FtpServer {
public:
    int port = 21;
    string rootdir = ".";

    void chroot(const string &dir);

    void set_port(int port);

    static void work(FtpSession *session);

    void start();
};

class FtpSession {
public:
    bool passive;
    CmdHandler handler;
    SocketPtr ctrl;
    shared_ptr<ServerSocket> srv_socket;
    string working_dir = "/";
    Command cur_cmd;
    int clt_id;
    char buf[1024 * 1024];

    FtpSession(const SocketPtr &socket, int clt_id = -1);

    void close_data_conn(SocketPtr socket, const string &msg);

    SocketPtr get_data_conn(const string &msg);

    string safe_path(const string &fpath, bool allow_nonexist = false);

    void run();

    void execute();
};

int main(int argc, char **argv);