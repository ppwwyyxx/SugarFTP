import "string"

using namespace std

void print_debug(s: const string&)
    printf("\x1b[32m")
    printf("%s", s.c_str())
    printf("\x1b[0m\n")

[public]
class FTPExc
    msg: string
    FTPExc(s: const string&)
        @msg = s

string exec(cmd: const string&)
    pipe: FILE* = popen(cmd.c_str(), "r")
    buf: char[1024]
    ret: string
    while !feof(pipe)
        ret += buf if fgets(buf, 128, pipe)
    pclose(pipe)
    return ret
