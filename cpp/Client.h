#pragma once

class StopCommand;
class FtpClient;

#include "iostream"
#include "cstdio"
#include "cstdlib"
#include "unistd.h"
#include "Socket.h"
#include "Command.h"

using namespace std;

class StopCommand {
};

class FtpClient {
public:
    SocketPtr ctrl;
    CmdHandler handler;
    char buf[1024 * 1024];

    FtpClient(SocketPtr socket);

    void quit();

    Command cmd_send(const string &cmd);

    Command response();

    SocketPtr open_data_conn();

    void start();
};

int main(int argc, char **argv);