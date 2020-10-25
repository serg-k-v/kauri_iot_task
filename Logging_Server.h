#include "ace/FILE_IO.h"
#include "ace/FILE_Connector.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"

// class ACE_SOCK_Stream;

class Logging_Server {
public:
    // Template Method that runs logging server's event loop.
    virtual int run (int argc, char const *argv[]);
protected:
    // The following four methods are ' 'hooks' ' that can be
    // overridden by subclasses
    virtual int open(u_short port = 0);
    virtual int wait_for_multiple_events() { return 0; }
    virtual int handle_connections() = 0;
    virtual int handle_data(ACE_SOCK_Stream* = 0) = 0;
    // This helper method can be used by the hook methods.
    int make_log_file(ACE_FILE_IO&, ACE_SOCK_Stream* = 0) ;
    // Close the socket endpoint and shutdown ACE.
    virtual ~Logging_Server() { acceptor_.close(); }

    ACE_SOCK_Acceptor &acceptor() { return acceptor_; }
private :
    ACE_SOCK_Acceptor acceptor_;    // Socket acceptor endpoint
};

int Logging_Server::run(int argc, char const *argv[])
{
    if (open(argc > 1 ? atoi(argv[1]) : 0) == -1)
        return -1;

    for (;;) {
        if (wait_for_multiple_events() == -1) 
            return -1;
        if (handle_connections() == -1) 
            return -1;
        if (handle_data() == -1) 
            return -1;
    }

    return 0;
}

int Logging_Server::make_log_file(ACE_FILE_IO &logging_file, ACE_SOCK_Stream *logging_peer)
{
    char filename[MAXHOSTNAMELEN + sizeof(".log")];

    if (logging_peer != 0) { // Use client host name as file name.
        ACE_INET_Addr logging_peer_addr;
        logging_peer->get_remote_addr(logging_peer_addr);
        logging_peer_addr.get_host_name(filename, MAXHOSTNAMELEN);
        strcat (filename, ".log" ) ;
    }
    else
        strcpy (filename, "logging_server.log");

    ACE_FILE_Connector connector;
    return connector.connect(logging_file, ACE_FILE_Addr(filename),
                             0, // No time-out.
                             ACE_Addr::sap_any, // Ignored.
                             0, // Don't try to reuse the addr.
                             O_RDWR | O_CREAT | O_APPEND,
                             ACE_DEFAULT_FILE_PERMS);
}

int Logging_Server::open(u_short logger_port)
{
    // Raises the number of available socket handles to
    // the maximum supported by the OS platform.
    ACE::set_handle_limit();
    ACE_INET_Addr server_addr;
    int result;

    if (logger_port != 0)
        result = server_addr.set(logger_port, INADDR_ANY);
    else
        result = server_addr.set("ace_logger", INADDR_ANY);

    if (result == -1)
        return -1;

    // Start listening and enable reuse of listen address for quick restarts.
    return acceptor_.open(server_addr, 1);
}