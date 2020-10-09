/*
 * Cpp server version 1: Server implemented with epoll.
 */

#include <iostream>
#include <string.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

#define MAX_OPEN 2048

void reverse_string(char* origin_str, char* target_str, int max_len = 255) {
    if (origin_str == nullptr || '\0' == *origin_str) {
        target_str[0] = '\0';
        return ;
    }

    int str_len = 0;
    char* lptr = origin_str;
    char* rptr = origin_str;
    while (*rptr != '\0' && str_len++ < max_len) {
        rptr++;
    }

    *rptr = '\0';
    rptr--;
    while (lptr < rptr) {
        char tmp = *lptr;
        *lptr = *rptr;
        *rptr = tmp;
        lptr++;
        rptr--;
    }

    strcpy(target_str, origin_str);
    cout << target_str << endl;
}

void process_func(std::function<void (char*, char*, int)> fn, char* recv_buf, char* send_buf) {
    fn(recv_buf, send_buf, 255);
}

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        cout << "Error 001: create server socket failed." << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cout << "Error 002: bind failed." << endl;
        return -1;
    }

    if (listen(listen_sock, MAX_OPEN) == -1) {
        cout << "Error 003: listen failed. This might be caused by: the server was exitted by force and the [listening port] is in the TIME_WAIT status." << endl;
        return -1;
    }

    size_t epfd = epoll_create(MAX_OPEN);
    if (epfd < 0) {
        cout << "Error 003: create epoll failed." << endl;
        return -1;
    }

    struct epoll_event temp, events[MAX_OPEN];
    temp.events = EPOLLIN | EPOLLET;
    temp.data.fd = listen_sock;
    size_t ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &temp);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &temp) < 0) {
        cout << "Error 004: add server_listen_socket to epoll failed." << endl;
        return -1;
    }

    char recv_buf[255];
    char send_buf[255];

    int remain_connections = 0;

    cout << "Waiting for new connection..." << endl;

    while (true) {
        size_t nready = epoll_wait(epfd, events, MAX_OPEN, -1);
        if (nready < 0) {
            cout << "Error 101: epoll wait failed." << endl;
            return -1;
        }

        for (int i = 0; i < nready; ++i) {
            if (events[i].data.fd == listen_sock) {
                socklen_t client_len = sizeof(client_addr);
                int conn = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);

                temp.events = EPOLLIN | EPOLLET;
                temp.data.fd = conn;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &temp) < 0) {
                    cout << "Error 102: add client socket to epoll failed." << endl;
                    return -1;
                }

            }
            else if (events[i].events & EPOLLIN) {
                int sockfd = events[i].data.fd;
                memset(recv_buf, 0, sizeof(recv_buf));
                int recv_len = read(sockfd, recv_buf, 255);
                recv_buf[recv_len] = '\0';
                
                // if connection closed by client.
                if (recv_len == 0 || strcmp(recv_buf, "exit") == 0) {
                    remain_connections -= 1;
                    cout << "[Server] Client leave, remain connections: " << remain_connections << endl;
                    close(sockfd);
                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr) < 0) {
                        cout << "Error 103: remove client socket to epoll failed." << endl;
                        return -1;
                    }
                }
                else {
                    process_func(reverse_string, recv_buf, send_buf);
                    send(sockfd, recv_buf, sizeof(recv_buf), 0);
                }

            }
        }
    }

    close(listen_sock);
}