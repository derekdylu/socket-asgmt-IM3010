#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

using namespace std;

struct sockaddr_in server;

int main(int argc, char *argv[])
{

    int socket_desc;
    socket_desc = socket(AF_INET, SOCK_STREAM, 0); // IPv4

    // create socket
    if (socket_desc == -1)
    {
        cout << "Could not creat socket" << endl;
    }

    server.sin_addr.s_addr = inet_addr("74.125.235.20");
    server.sin_family = AF_INET;
    server.sin_port = htons(80);

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("connect error");
        return 1;
    }

    puts("Connected");

    return 0;
}