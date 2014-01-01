#pragma once

class FTPExc;

#include "string"

using namespace std;

void print_debug(const string &s);

class FTPExc {
public:
    string msg;

    FTPExc(const string &s);
};

string exec(const string &cmd);