#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

    int socket_desc;
    struct sockaddr_in server;
    char *message, client_input[2000], server_reply[2000];

    // create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0); // IPv4

    if (socket_desc == -1)
    {
        printf("Could not creat socket");
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    // connect to remote server
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Connection error");
        return 1;
    }

    puts("Connected");

    while (1)
    {
        scanf("%s", client_input);
        message = client_input;

        // send some data
        if (send(socket_desc, message, strlen(message), 0) < 0)
        {
            puts("Sent failed");
            return 1;
        }
        puts("Data Sent\n");

        //Receive a reply from the server
        if (recv(socket_desc, server_reply, 2000, 0) < 0)
        {
            puts("Received failed");
        }
        puts("<Reply received>\n");
        puts(server_reply);

        // read(socket_desc, server_reply, 2000);
        puts("--------------------\n");

        /*
        if (message == "Exit")
        {
            break;
        }
        */
    }

    close(socket_desc);

    return 0;
}