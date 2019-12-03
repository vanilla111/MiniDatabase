#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#include "server/server.h"
#include "global.h"

using namespace std;

Server::Server(Interpreter *interpreter) {
    this->interpreter = interpreter;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &address.sin_addr);
    address.sin_port = htons(SERVER_PORT);
    listenFD = socket(PF_INET, SOCK_STREAM, 0);
    // printf("ip: %s, port: %d", SERVER_IP, SERVER_PORT);
    assert(listenFD >= 0);
    reuse = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = bind(listenFD, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    listen(listenFD, 5);
    assert(ret != -1);

    clients = new client_data[FD_LIMIT];
    clientCounter = 0;
    for (auto & fd : fds) {
        fd.fd = -1; // 初始化polled结构体
        fd.events = 0;
    }
    fds[0].fd = listenFD;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;
}

void Server::start() {
    while (true) {
        ret = poll(fds, clientCounter + 1, -1);
        if (ret < 0) {
            // cerr << "ERROR: [Server::start] Poll failed!" << endl;
            printf("ERROR: [Server::start] Poll failed!");
            break;
        }

        for (int i = 0; i < clientCounter + 1; ++i) {
            if (fds[i].fd == listenFD && (fds[i].revents & POLLIN)) {
                struct sockaddr_in clientAddress;
                socklen_t clientAddrLength = sizeof(clientAddress);
                int connfd = accept(listenFD, (struct sockaddr*)&clientAddress, &clientAddrLength);
                if (connfd < 0) {
                    // cerr << "ERROR: [Server::start] There is something wrong, and errno is" << errno << endl;
                    printf("ERROR: [Server::start] There is something wrong, and errno is %d", errno);
                    continue;
                }
                // 请求太多，关闭新的链接
                if (clientCounter >= CLIENT_LIMIT) {
                    const char *info = "WARN: There are to many client.";
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                // 接入新的客户端，修改fds和user数组
                clientCounter++;
                clients[connfd].address = clientAddress;
                setNonblocking(connfd);
                fds[clientCounter].fd = connfd;
                fds[clientCounter].events = POLLIN | POLLHUP | POLLERR;
                fds[clientCounter].revents = 0;
                // cout << "INFO: Comes a new client, now have " << clientCounter << " clients" << endl;
            } else if (fds[i].revents & POLLERR) {
                // cout << "WARN: Get an error from " << fds[i].fd << endl;
                printf("WARN: Get an error from %d", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t len = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0) {
                    // cout << "WARN: Get socket option failed!" << endl;
                    printf("WARN: Get socket option failed!");
                }
                continue;
            } else if (fds[i].revents & POLLHUP) {
                // 客户端关闭链接
                clients[fds[i].fd] = clients[fds[clientCounter].fd];
                close(fds[i].fd);
                fds[i] = fds[clientCounter];
                i--; clientCounter--;
                // cout << "INFO: A client left." << endl;
            } else if (fds[i].revents & POLLIN) {
                int connfd = fds[i].fd;
                memset(clients[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, clients[connfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %d bytes of client data: %s from %d\n", ret, clients[connfd].buf, connfd);
                if (ret <= 0) {
                    // 读取数据失败，关闭链接
                    if (errno != EAGAIN) {
                        close(connfd);
                        clients[fds[i].fd] = clients[fds[clientCounter].fd];
                        fds[i] = fds[clientCounter];
                        i--; clientCounter--;
                    }
                } else {
                    // 接收到命令，开始解析
                    char sendBuf[SEND_BUFFER_SIZE + 1];
                    const char *filename = "send_buffer.tmp";
                    FILE *file = fopen(filename, "w");
                    fclose(file);
                    interpreter->execute(clients[connfd].buf, filename);
                    file = fopen(filename, "r");
                    while ((fgets(sendBuf, SEND_BUFFER_SIZE, file)) != nullptr) {
                        send(connfd, sendBuf, strlen(sendBuf), 0);
                    }
                    fclose(file);
                }
            } else if (fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if (!clients[connfd].write_buf) continue;
                ret = send(connfd, clients[connfd].write_buf, strlen(clients[connfd].write_buf), 0);
                clients[connfd].write_buf = nullptr;
                fds[i].events |= ~POLLOUT;
                fds[i].revents |= POLLIN;
            }
        }
    }
}

Server::~Server() {
    delete [] clients;
    close(listenFD);
}

int Server::setNonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}