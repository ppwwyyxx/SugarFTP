import "iostream"
       "cstdio"
       "cstdlib"
       "unistd.h"

import "Socket.h"
       "Command.h"

using namespace std

class StopCommand

[public]
class FtpClient
    ctrl: SocketPtr
    handler: CmdHandler
    buf: char[1024 * 1024]

    FtpClient(socket: SocketPtr)
        @ctrl = socket
        @handler = CmdHandler(socket)
        @response()
        @cmd_send("USER anonymous")
        @cmd_send("TYPE I")

    void quit() = @cmd_send("QUIT")

    Command cmd_send(cmd: const string&)
        print_debug("--> " + cmd)
        @ctrl->send((cmd + "\r\n").c_str(), cmd.length() + 2)
        return @response()

    Command response()
        cmd := @handler.recv()
        if cmd.cmd[0] != '1' and cmd.cmd[0] != '2'
            print_debug("Bad response: " + cmd.to_string())
            throw(StopCommand())
        print_debug("<-- " + cmd.to_string())
        return cmd

    SocketPtr open_pasv_data_conn()
        cmd := cmd_send("PASV")
        h0, h1, h2, h3, p0, p1: int
        if sscanf(
            cmd.arg.substr(cmd.arg.rfind(' ') + 1).c_str(),
            "(%d, %d, %d, %d, %d, %d)",
            &h0, &h1, &h2, &h3, &p0, &p1
           ) != 6
            print_debug("Bad response for PASV: " + cmd.to_string())
            throw(StopCommand())
        return TCPSocket::connect(
            TCPSocket::ip_addr(h0, h1, h2, h3).c_str()
            to_string(p0 * 256 + p1).c_str()
            )

    void start()
        loop
            cmd, arg: string
            try
                printf("SugarFTP> ")
                s: string
                if not getline(cin, s)
                    return
                iss : istringstream(s)
                iss >> cmd >> arg
                switch
                    when cmd == "help"
                        cout << "Available command: ls / cd / q(quit) / rm / put / get / help" << endl
                    when cmd == "ls"
                        data_conn := @open_pasv_data_conn()
                        @cmd_send("LIST " + arg)
                        loop
                            s := data_conn->recv(@buf, sizeof(@buf))
                            break if s <= 0
                            fwrite(@buf, 1, s, stdout)
                            fflush(stdout)
                        @response()
                    when cmd == "cd"
                        @cmd_send("CWD " + arg)
                    when cmd == "rm"
                        @cmd_send("DELE " + arg)
                    when cmd == "put"
                        fin := fopen(arg.c_str(), "rb")
                        if not fin
                            print_debug("Failed to open file" + arg)
                            throw(StopCommand())
                        finally fclose(fin)

                        data_conn := @open_pasv_data_conn()
                        @cmd_send(string("STOR ") + basename(strdupa(arg.c_str())))
                        loop
                            size := fread(@buf, 1, sizeof(@buf), fin)
                            break if size <= 0
                            data_conn->send(@buf, size)
                        data_conn->close()
                        @response()
                    when cmd == "get"
                        fout := fopen(arg.c_str(), "wb")
                        if not fout
                            print_debug("Failed to open file" + arg)
                            throw(StopCommand())
                        finally fclose(fout)
                        try
                            data_conn := @open_pasv_data_conn()
                            @cmd_send("RETR " + arg)
                            loop
                                size := data_conn->recv(@buf, sizeof(@buf))
                                break if size <= 0
                                fwrite(@buf, 1, size, fout)
                            data_conn->close()
                            @response()
                        catch e: FTPExc&
                            unlink(arg.c_str())
                            throw
                    when cmd == "q" or cmd == "quit"
                        return
                    else
                        cout << "Command not supported, enter 'help' to see help" << endl
            catch a: StopCommand
                continue

int main(argc: int, argv: char**)
    if argc != 3
        cerr << "Usage: " << argv[0] << " <host> <port>" << endl
        return -1
    try
        client : FtpClient(TCPSocket::connect(argv[1], argv[2]))
        finally client.quit()
        client.start()
    catch e: FTPExc
        cerr << e.msg << endl
