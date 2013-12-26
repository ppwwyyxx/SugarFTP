import "iostream"
       "thread"
       "cstdlib"
       "sys/stat.h"
       "unistd.h"
       "cstring"

import "Common.h"
       "Socket.h"
       "Command.h"

using namespace std

server: FtpServer*

class StopCommand
class Quit

[public]
class FtpServer
    port: int = 21
    rootdir: string = "."

    void chroot(dir: const string&)
        p := realpath(dir.c_str(), nullptr)
        throw(FTPExc("Failed to chroot to " + dir + " : " + strerror(errno))) if not p
        @rootdir.assign(p)
        free(p)
        @rootdir.append("/") if @rootdir.back() != '/'

    void set_port(port: int) = @port = port

    [static]
    void work(session: FtpSession *)
        finally delete(session)
        try
            session->run()
            print_debug("Session " + session->get_peerinfo() + " exited")
        catch e: FTPExc
            print_debug("Exception Happened: " + e.msg)

    void start()
        socket: ServerSocket(@port)
        print_debug("Server listening on " + TCPSocket::format_addr(socket.get_local_addr()) + ":" + to_string(socket.get_local_port()))
        id := 0
        loop
            client := new FtpSession(socket.accept(), id++)
            job := thread(FtpServer::work, client)
            job.detach()

[public]
class FtpSession
    passive: bool
    handler: CmdHandler
    ctrl: SocketPtr
    srv_socket: shared_ptr<ServerSocket>
    working_dir: string = "/"
    cur_cmd: Command
    clt_id: int
    buf: char[1024 * 1024]

    FtpSession(socket: const SocketPtr&, clt_id: int = -1)
        @handler = CmdHandler(socket)
        @ctrl = socket
        @clt_id = clt_id
        @ctrl->enable_timeout()
        print_debug("new client: " + @ctrl->get_peerinfo() + " [as " + @get_peerinfo() + " ]")

    void close_data_conn(socket: SocketPtr, msg: const string&)
        socket->close()
        @handler.send("226", msg)

    SocketPtr get_data_conn(msg: const string&)
        if not @passive
            @handler.send("425", "use PASV first")
            throw(StopCommand())
        ret := @srv_socket->accept()
        @srv_socket.reset()
        @passive = false
        @handler.send("125", msg)
        ret->enable_timeout()
        return ret

    string safe_path(fpath: const string&, allow_nonexist : bool = false)
        rootdir: string& = server->rootdir
        dirname, basename, query_path: string
        if allow_nonexist
            basename = fpath
            i := fpath.rfind('/')
            if i != string::npos
                dirname.assign(fpath.c_str(), i)
                basename.assign(fpath.c_str() + i)
            query_path = dirname
        else
            query_path = fpath
        retc: char*
        if query_path[0] == '/'
            retc = realpath((rootdir + query_path).c_str(), nullptr)
        else
            retc = realpath((rootdir + working_dir + "/" + query_path).c_str(), nullptr)
        if not retc
            @handler.send("550", "bad file path")
            throw(StopCommand())

        ret: string(retc)
        free(retc)
        if ret.substr(0, rootdir.length()) != rootdir and ret != rootdir.substr(0, rootdir.length() - 1)
            @handler.send("550", "bad file path")
            throw(StopCommand())
        if allow_nonexist
            ret.append("/") if ret.back() != '/'
            ret.append(basename)
        return ret

    void run()
        try
            @handler.send("220", "Sugar Ftp")
            loop
                try
                    execute()
                catch e: StopCommand
                    continue
        catch e: Quit
            return

    [const]
    string get_peerinfo()
        if @clt_id >= 0
            buf: static thread_local char[20]
            sprintf(buf, "%d", @clt_id)
            return string(buf)
        return @ctrl->get_peerinfo()

    void execute()
        @cur_cmd = @handler.recv()
        print_debug("Client " + @get_peerinfo() + ": " + @cur_cmd.to_string())
        cmd := @cur_cmd.cmd
        switch
            when cmd == "USER"
                @handler.send("230", "welcome!")
            when cmd == "TYPE"
                @handler.send("200", "binary")
            when cmd == "QUIT"
                @handler.send("221", "Goodbye")
                throw(Quit())
            when cmd == "FEAT"
                @handler.send("211", "")
            when cmd == "ALLO"
                @handler.send("202", "ALLO!")
            when cmd == "PWD"
                @handler.send("257", "\"" + @working_dir + "\"")
            when cmd == "PASV"
                @passive = true
                addr := @ctrl->get_local_addr()
                tmp : shared_ptr<ServerSocket>(new ServerSocket(0))
                @srv_socket = tmp
                @srv_socket->enable_timeout()
                port := @srv_socket->get_local_port()
                @handler.send(
                    "227"
                    "Entering Passive Mode (" + TCPSocket::format_addr(addr, ',') + "," + to_string(port >> 8) + "," + to_string(port & 0xff) + ")."
                    )
            when cmd == "LIST"
                path := @cur_cmd.arg
                // ignore all options
                if path[0] == '-'
                    path.erase(begin(path), find_if(begin(path), end(path), (x: char) -> (isspace(x))))
                path = @safe_path(path)
                ls := "[[ -f \"" + path + "\" ]] && ls -al " + path + " || ls -al " + path + " | tail -n +2"
                data_conn := @get_data_conn("start listing")
                ret := exec(ls)
                data_conn->send(ret.c_str(), ret.length())
                @close_data_conn(data_conn, "finished litsing")
            when cmd == "CWD"
                new_dir := @safe_path(@cur_cmd.arg)
                t_stat : struct stat
                if global::stat(new_dir.c_str(), &t_stat) or not S_ISDIR(t_stat.st_mode)
                    @handler.send("550", "failed to chdir")
                    return
                @working_dir = new_dir
                @working_dir.erase(0, server->rootdir.length() - 1)
                @handler.send("250", "working dir changed to " + @working_dir)
            when cmd == "RETR"
                realpath := @safe_path(@cur_cmd.arg)
                fin: FILE* = fopen(realpath.c_str(), "rb")
                if not fin
                    @handler.send("550", "failed to open file")
                    return
                finally fclose(fin)
                data_conn := @get_data_conn("going to transfer " + @cur_cmd.arg)
                loop
                    size := fread(@buf, 1, sizeof(@buf), fin)
                    break if size <= 0
                    data_conn->send(@buf, size)
                @close_data_conn(data_conn, "transfer completed")
            when cmd == "STOR"
                realpath := @safe_path(@cur_cmd.arg, 1)
                fout: FILE* = fopen(realpath.c_str(), "wb")
                if not fout
                    @handler.send("553", "failed to open file for write")
                    return
                finally fclose(fout)
                data_conn := @get_data_conn("OK to transfer")
                totsize := 0
                loop
                    size := data_conn->recv(@buf, sizeof(@buf))
                    break if size <= 0
                    totsize += size
                    fwrite(@buf, 1, size, fout)
                print_debug("client " + @get_peerinfo() + ": upload file " + realpath + " size: " + to_string(totsize))
                @close_data_conn(data_conn, "transfer completed")
            when cmd == "SIZE"
                realpath := @safe_path(@cur_cmd.arg)
                stat: struct stat
                if global::stat(realpath.c_str(), &stat)
                    @handler.send("550", "failed to get file size")
                @handler.send("213", to_string(stat.st_size))
            else
                @handler.send("502", "unimplemented command")


int main(argc: int, argv: char**)
    s: FtpServer
    server = &s
    if argc == 1
        cerr << "Usage: " << argv[0] << " <root dir> [port = 8888]" << endl
        return -1
    try
        if argc >= 2
            server->chroot(argv[1])
        if argc >= 3
            server->set_port(atoi(argv[2]))
        else
            server->set_port(8888)
        server->start()
    catch e: FTPExc
        cout << e.msg << endl
