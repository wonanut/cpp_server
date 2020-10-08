#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        cout << "Error 401: create client socket failed." << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Error 402: connect failed." << endl;
        return -1;
    }

    cout << "Success: connected to server." << endl;

    char send_buf[255];
    char recv_buf[255];
    while (cin.getline(send_buf, 255)) {
        send(client, send_buf, strlen(send_buf), 0);
        if (strcmp(send_buf, "exit") == 0) {
            cout << "Exit: disconnecting with server." << endl;
            break;
        }

        memset(recv_buf, 0, sizeof(recv_buf));
        int recv_len = recv(client, recv_buf, sizeof(recv_buf), 0);
        recv_buf[recv_len] = '\0';
        cout << "[Server]: " << recv_buf << endl;
    }

    close(client);
}