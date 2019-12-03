#ifndef WANGSQL_SERVER_H
#define WANGSQL_SERVER_H

#define CLIENT_LIMIT 5
#define BUFFER_SIZE 256
#define SEND_BUFFER_SIZE 4096
#define FD_LIMIT 65536

#include <netinet/in.h>
#include <poll.h>
#include "interpreter/interpreter.h"

struct client_data
{
    sockaddr_in address;
    char *write_buf;
    char buf[BUFFER_SIZE];
};

class Server
{
public:
    Server(Interpreter *interpreter);

    ~Server();

    void start();

private:
    client_data *clients;

    int listenFD;

    int reuse;

    int ret;

    int clientCounter;

    Interpreter *interpreter;

    pollfd fds[CLIENT_LIMIT];

    struct sockaddr_in address;

    int setNonblocking(int fd);
};

#endif //WANGSQL_SERVER_H
