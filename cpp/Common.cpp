#include "Common.h"

void print_debug(const string &s) {
    printf("\x1b[32m");
    printf("%s", s.c_str());
    printf("\x1b[0m\n");
}

FTPExc::FTPExc(const string &s) {
    (this->msg) = s;
}

string exec(const string &cmd) {
    FILE *pipe = popen(cmd.c_str(), "r");
    char buf[1024];
    string ret;
    while (!feof(pipe)) {
        if (fgets(buf, 128, pipe)) {
            ret += buf;
        }
    }
    pclose(pipe);
    return ret;
}