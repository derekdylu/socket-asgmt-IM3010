#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

#include <pthread.h> //for threading , link with lpthread

#include <string.h>

void *connection_handler(void *);
void regist(int sock, char *cmd);
void login(int sock, char *client_message);
void list(int sock);
void trans(int sock, char *client_message);
void debug(int sock, char *client_message);

typedef struct User
{
    char *username;
    int port;    // if not login = -1
    int balance; // initially $10,000
    int login;   // 0 not, 1 login
} User;

void addUser(struct User *p, struct User a, int *usrCnt);
void dropUser(struct User *p, char *username, int *usrCnt);
int findUser(struct User *p, char *username, int usrCnt);
int duplicatePort(struct User *p, int port, int usrCnt);
void listUser(struct User *p, int usrCnt);

User users[100] = {};
int usrCnt = 0;

int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;
    char *message;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    //Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("bind failed");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        puts("Connection accepted");

        //Reply to the client
        //message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
        //write(new_socket, message, strlen(message));

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *)new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join(sniffer_thread, NULL); //<- this make it single threading
        puts("Handler assigned");
    }

    if (new_socket < 0)
    {
        perror("accept failed");
        return 1;
    }

    return 0;
}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int *)socket_desc;
    int read_size;
    char *message, client_message[2000];

    //Send some messages to the client
    //message = "Greetings! I am your connection handler\n";
    //write(sock, message, strlen(message));

    //Receive a message from client
    while ((read_size = recv(sock, client_message, 2000, 0)) > 0)
    {
        if (strcmp(client_message, "List") == 0)
        {
            list(sock);
            continue;
        }
        else if (strstr(client_message, "REGISTER#") != NULL)
        {
            regist(sock, client_message);
            continue;
        }
        else
        {
            login(sock, client_message);
            continue;
        }

        //write(sock, client_message, strlen(client_message)); // send msg back to client
    }

    if (read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if (read_size == -1)
    {
        perror("recv failed");
    }

    //Free the socket pointer
    free(socket_desc);

    return 0;
}

void regist(int sock, char *client_message)
{
    char callback[100] = "Register > ";
    char username[100];
    sscanf(client_message, "%*[^#]#%[^\n]", username);
    strcat(callback, username);
    puts(callback);
    char *message;

    if (findUser(users, username, usrCnt) == -1) // if there's no duplicate
    {
        puts("Register succeeded");
        message = "100 OK\n";
        write(sock, message, strlen(message));
        User newUser = {username, -1, 10000, 0};
        addUser(users, newUser, &usrCnt);
    }
    else
    {
        puts("Account already registered");
        message = "210 FAIL\n";
        write(sock, message, strlen(message));
    }

    return;
}

void login(int sock, char *client_message)
{
    char callback[2000] = "Login > ";
    char username[100];
    char ports[100];
    int port;
    sscanf(client_message, "%[^#]#%[^\n]", username, ports);
    strcat(callback, username);
    puts(callback);
    char *message;

    int userIdx = findUser(users, username, usrCnt);

    if (userIdx == -1)
    {
        puts("Username not found");
        message = "220 AUTH_FAIL\n";
        write(sock, message, strlen(message));
        return;
    }

    users[userIdx].login = 1; // set login to 1
    message = "successfully logged in. this will show list()\n";
    write(sock, message, strlen(message));
}

void list(int sock)
{
    char *message;
    message = "list";
    write(sock, message, strlen(message));
    listUser(users, usrCnt);
}

void trans(int sock, char *client_message) {}
void debug(int sock, char *client_message)
{
    write(sock, client_message, strlen(client_message));
}

void addUser(struct User *p, struct User a, int *usrCnt)
{
    {
        if (*usrCnt < 100) // limit
        {
            p[*usrCnt] = a;
            *usrCnt += 1;
        }
    }
}

void dropUser(struct User *p, char *username, int *usrCnt) {}

int findUser(struct User *p, char *username, int usrCnt)
{
    for (int i = 0; i < usrCnt; i++)
    {
        if (p[i].username == username)
        {
            return i;
        }
    }
    return -1;
}

int duplicatePort(struct User *p, int port, int usrCnt) {}

void listUser(struct User *p, int usrCnt)
{
    if (usrCnt == 0)
    {
        puts("no users");
    }

    printf("%d\n", usrCnt);

    for (int i = 0; i < usrCnt; i++)
    {
        printf("%d\n", i);
        puts(p[i].username);
        printf("%d\n", p[i].port);
        printf("%d\n", p[i].balance);
        printf("%d\n", p[i].login);
        puts("-");
    }
    puts("-----");
}