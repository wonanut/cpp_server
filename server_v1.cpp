/*
 * Cpp server version 1: One-thread per connection.
 */

#include <iostream>
#include <string.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
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

void thread_func(int conn, std::function<void (char*, char*, int)> func) {
    char recv_buf[255];
    char send_buf[255];
    while (true) {
        memset(recv_buf, 0, sizeof(recv_buf));
        int recv_len = recv(conn, recv_buf, sizeof(recv_buf), 0);
        recv_buf[recv_len] = '\0';
        if (strcmp(recv_buf, "exit") == 0) {
            cout << "Exit: client exit." << endl;
            break;
        }
        cout << "[Client] " << recv_buf << endl;

        process_func(func, recv_buf, send_buf);
        cout << "[Server] " << send_buf << endl;
        send(conn, send_buf, sizeof(send_buf), 0);
    }

    close(conn);
}

int main() {
    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server == -1) {
        cout << "Error 001: create server socket failed." << endl;
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_server, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cout << "Error 002: bind failed." << endl;
        return -1;
    }

    if (listen(socket_server, 5) == -1) {
        cout << "Error 003: listen failed." << endl;
        return -1;
    }

    int conn;
    std::function<void (char*, char*, int)> func = reverse_string;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    while (true) {
        cout << "Waiting for new connection..." << endl;
        conn = accept(socket_server, (struct sockaddr*)&client_addr, &client_addr_len);
        if (conn < 0) {
            cout << "Error 101: accept failed." << endl;
            return -1;
        }

        std::thread t(thread_func, conn, reverse_string);
        t.detach();
    }
}