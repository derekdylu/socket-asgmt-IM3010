#include <iostream>
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

#include <pthread.h> //for threading , link with lpthread

#include <string.h>
#include <string>

using namespace std;

void *connection_handler(void *);
void regist(int sock, char *cmd);
void login(int sock, char *client_message);
void trans(int sock, char *client_message);
void debug(int sock, char *client_message);

typedef struct User
{
    string username;
    int port;    // if not login = -1
    int balance; // initially $10,000
    int login;   // 0 not, 1 login
    int usock;   // to store sock
} User;

void addUser(struct User *p, struct User a, int *usrCnt);
void dropUser(struct User *p, int sock, int *usrCnt);
int findUser(struct User *p, char *username);
int duplicatePort(struct User *p, int port);
void listUser(struct User *p, int sock);
int getBalance(struct User *p, int sock);

User users[20] = {};
int usrCnt = 0;
string ip;

int threadsCnt = 0;

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
    ip = argv[1];
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(atoi(argv[2]));

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
        new_sock = (int *)malloc(1);
        *new_sock = new_socket;

        if (threadsCnt < 3)
        {
            int ptc = pthread_create(&sniffer_thread, NULL, connection_handler, (void *)new_sock);
            if (ptc < 0)
            {
                perror("could not create thread");
                return 1;
            }
            else
            {
                threadsCnt++;
            }
            cout << threadsCnt << endl;
        }
        else
        {
            string waitingMessage = "Limit 3 threads at the time.";
            write(*(int *)new_sock, waitingMessage.c_str(), sizeof(waitingMessage));
            puts("Limit 3 threads at the time.");
        }
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
    char client_message[2000];
    string message;

    //Send some messages to the client
    puts("Connection handler assigned.");

    //Receive a message from client
    while ((read_size = recv(sock, client_message, 2000, 0)) > 0)
    {
        if (strcmp(client_message, "Exit") == 0)
        {
            message = "Bye\n";
            write(sock, message.c_str(), sizeof(message));
            dropUser(users, sock, &usrCnt);
            continue;
        }
        else if (strcmp(client_message, "List") == 0)
        {
            listUser(users, sock);
            continue;
        }
        else if (strstr(client_message, "REGISTER#") != NULL)
        {
            regist(sock, client_message);
            continue;
        }
        else
        {
            int hashtag = 0;
            for (int j = 0; j < strlen(client_message); j++)
            {
                if (client_message[j] == '#')
                {
                    hashtag++;
                }
                if (hashtag == 2)
                {
                    break;
                }
            }

            if (hashtag == 0)
            {
                message = "bad input";
                write(sock, message.c_str(), sizeof(message));
            }
            else if (hashtag == 1)
            {
                login(sock, client_message);
                continue;
            }
            else if (hashtag == 2)
            {
                trans(sock, client_message);
            }
        }
    }

    if (read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
        threadsCnt--;
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
    string message;

    if (findUser(users, username) == -1) // if there's no duplicate
    {
        puts("Register succeeded");
        message = "100 OK\n";
        write(sock, message.c_str(), sizeof(message));
        User newUser = {username, -1, 10000, 0, sock};
        addUser(users, newUser, &usrCnt);
    }
    else
    {
        puts("Account already registered");
        message = "210 FAIL\n";
        write(sock, message.c_str(), sizeof(message));
    }

    return;
}

void login(int sock, char *client_message)
{
    char callback[2000] = "Login > ";
    char username[100];
    char ports[100];

    sscanf(client_message, "%[^#]#%[^\n]", username, ports);
    strcat(callback, username);
    puts(callback);
    string message;
    string port(ports);

    int userIdx = findUser(users, username);

    if (userIdx == -1)
    {
        puts("Username not found");
        message = "220 AUTH_FAIL\n";
        write(sock, message.c_str(), sizeof(message));
        return;
    }

    /*
    if (usrCnt > 3)
    {
        puts("limit 3 users at the time");
        message = "220 AUTH_FAIL\n";
        write(sock, message.c_str(), sizeof(message));
        return;
    }
    */

    users[userIdx].login = 1; // set login to 1
    users[userIdx].port = stoi(port);
    puts("User logged in successfully");
    listUser(users, sock);
}

void trans(int sock, char *client_message)
{
    char callback[2000] = "Transaction > from:";
    char usernameA[100];
    char usernameB[100];
    char amount[100];

    sscanf(client_message, "%[^#]#%[^#]#%[^\n]", usernameA, amount, usernameB);
    strcat(callback, usernameA);
    strcat(callback, " to: ");
    strcat(callback, usernameB);
    strcat(callback, " amunt= ");
    strcat(callback, amount);
    puts(callback);
    string message;
    string samount(amount);

    int aIdx = findUser(users, usernameA);
    int bIdx = findUser(users, usernameB);

    if (aIdx == -1 || bIdx == -1)
    {
        message = "account not exist, reject\n";
        write(sock, message.c_str(), sizeof(message));
        return;
    }

    puts("pass");
    users[aIdx].balance -= stoi(samount);
    users[bIdx].balance += stoi(samount);
}

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

void dropUser(struct User *p, int sock, int *usrCnt)
{
    int item;
    if (*usrCnt > 0)
    {
        for (int k = 0; k < *usrCnt; k++)
        {
            if (p[k].usock == sock)
            {
                item = k;
            }
        }
        int last_index = *usrCnt - 1;
        for (int i = item; i < last_index; i++)
        {
            p[i] = p[i + 1];
        }
        *usrCnt -= 1;
    }
}

int findUser(struct User *p, char *username)
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

int getBalance(struct User *p, int sock)
{
    for (int i = 0; i < usrCnt; i++)
    {
        if (p[i].usock == sock)
        {
            return p[i].balance;
        }
    }
    return -1;
}

int getOnlineCount(struct User *p, string &listing)
{
    int cnt = 0;
    for (int i = 0; i < usrCnt; i++)
    {
        if (p[i].login == 1)
        {
            listing.append(p[i].username);
            listing.append("#127.0.0.1#");
            listing.append(to_string(p[i].port));
            listing.append("\n");
            cnt++;
        }
    }
    return cnt;
}

int isLogIn(struct User *p, int sock)
{
    for (int i = 0; i < usrCnt; i++)
    {
        if (p[i].usock == sock)
        {
            return p[i].login;
        }
    }
    return -1;
}

void listUser(struct User *p, int sock)
{
    cout << "Sock: " << sock << " ask for list" << endl;

    string message;
    string listing = "";

    if (isLogIn(users, sock) < 1)
    {
        message = "Please log in first.";
        write(sock, message.c_str(), sizeof(message));
        return;
    }

    if (usrCnt == 0)
    {
        puts("no users");
        return;
    }

    message = to_string(getBalance(users, sock));
    message.append("\n");
    // message.append("<ServerPublicKey>\n");
    message.append(to_string(getOnlineCount(users, listing)));
    // message.append("\n");
    // message.append(listing);
    // message.append("\n");

    for (int i = 0; i < usrCnt; i++)
    {
        if (p[i].login == 1)
        {
            message.append("\n");
            message.append(p[i].username);
            message.append("#127.0.0.1#");
            message.append(to_string(p[i].port));
        }
    }

    cout << message << endl;

    write(sock, message.c_str(), 2000);
}

int duplicatePort(struct User *p, int port) {}