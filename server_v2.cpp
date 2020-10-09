/*
 * Cpp server version 1: Server implemented with select.
 */

#include <iostream>
#include <string.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

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

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cout << "Error 002: bind failed." << endl;
        return -1;
    }

    if (listen(listen_sock, 5) == -1) {
        cout << "Error 003: listen failed. This might be caused by: the server was exitted by force and the [listening port] is in the TIME_WAIT status." << endl;
        return -1;
    }

    int maxfd = listen_sock;
    int maxi = -1;
    int nready;
    int client[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }

    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(listen_sock, &allset);
    
    char recv_buf[255];
    char send_buf[255];
    std::function<void (char*, char*, int)> func = reverse_string;

    timeval tv;
    tv.tv_sec = 0.01;
    tv.tv_usec = 0;

    int remain_connections = 0;

    cout << "Waiting for new connection..." << endl;

    while (true) {
        rset = allset;
        nready = select(maxfd + 1, &rset, nullptr, nullptr, nullptr);

        // if new connection arrived.
        if (FD_ISSET(listen_sock, &rset)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int conn = accept(listen_sock, (struct sockaddr*)&client_addr, &client_addr_len);

            int i = 0;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = conn;
                    break;
                }
            }

            if (i == FD_SETSIZE) {
                cout << "Error 102: connection out of FD_SETSIZE." << endl;
                return -1;
            }

            FD_SET(conn, &allset);
            remain_connections += 1;
            cout << "[Server] Client connected, remain connections: " << remain_connections << endl;

            // maxfd for select.
            if (conn > maxfd) {
                maxfd = conn;
            }

            // max index in client[] array;
            if (i > maxi) {
                maxi = i;
            }

            // if only the listen socket ready, quit the loop directly.
            if (--nready <= 0) {
                continue;
            }
        }

        // check all clients for data.
        for (int i = 0; i <= maxi; i++) {
            int sockfd = client[i];
            if (sockfd < 0) continue;
            if (FD_ISSET(sockfd, &rset)) {
                memset(recv_buf, 0, sizeof(recv_buf));
                int recv_len = read(sockfd, recv_buf, 255);
                recv_buf[recv_len] = '\0';
                
                // if connection closed by client.
                if (recv_len == 0 || strcmp(recv_buf, "exit") == 0) {
                    remain_connections -= 1;
                    cout << "[Server] Client leave, remain connections: " << remain_connections << endl;
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else {
                    process_func(reverse_string, recv_buf, send_buf);
                    send(sockfd, recv_buf, sizeof(recv_buf), 0);
                }

                if (--nready <= 0) {
                    break;
                }
            }
        }
    }
}