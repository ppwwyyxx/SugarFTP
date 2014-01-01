#pragma once

class Command;
class CmdHandler;

#include "string"
#include "algorithm"
#include "iostream"
#include "Socket.h"
#include "Common.h"

using namespace std;

class Command {
public:
    string cmd;
    string arg;

    string to_string() const;
};

class CmdHandler {
public:
    SocketPtr socket;

    CmdHandler();

    CmdHandler(SocketPtr socket);

    Command recv();

    void send(const string &cmd, const string &arg = "");
};