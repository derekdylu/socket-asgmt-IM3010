#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

#define MAX_LENGTH 2000

void *receive_thread(void *sd);
void receiving(int server_sd);
void sending();
void menu();
char server_reply[MAX_LENGTH];
int sd;

string myUser;

int main(int argc, char *argv[])
{
    //connect to server
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        printf("Couldn't create socket.\n");
    }
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Main program connection error.\n");
        return 1;
    }
    else
    {
        printf("Connected!\n");
    }

    int sd_t = socket(AF_INET, SOCK_STREAM, 0);
    if (sd_t == -1)
    {
        printf("Couldnt create socket.\n");
    }

    printf("[Login port number] > ");
    char port[10];
    scanf("%s", port);

    struct sockaddr_in client_;
    socklen_t addrlen = sizeof(client_);
    client_.sin_addr.s_addr = INADDR_ANY;
    client_.sin_family = AF_INET;
    client_.sin_port = htons(atoi((char *)port));

    if (bind(sd_t, (struct sockaddr *)&client_, sizeof(client_)) < 0)
    {
        cout << "Client bind failed: " << strerror(errno) << endl;
        return 1;
    };
    cout << "Client bind done" << endl;
    listen(sd_t, 5);

    // multi-thread receiving
    pthread_t tid;
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    pthread_create(&tid, NULL, receive_thread, &sd_t);

    string list; // usernames
    int command; // command

    menu();

    while (1)
    {
        scanf("%d", &command);
        if (command == 1) // register
        {
            printf("[Set username] > ");
            char user[20];
            scanf("%s", user);

            string msg;
            msg.append("REGISTER#");
            msg.append(user);
            send(sd, msg.c_str(), sizeof(msg), 0);
            printf("Wait for server to response...\n");

            if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
            {
                puts("Receiving failed.\n");
            }
            else
            {
                puts(server_reply);
            }
            bzero(server_reply, MAX_LENGTH);
        }
        else if (command == 2) // login
        {
            printf("[Username] > ");
            char user[20];
            scanf("%s", user);
            string msg;
            msg.append(user);
            msg.append("#");
            msg.append(port);
            send(sd, msg.c_str(), sizeof(msg), 0);
            printf("Wait for server to response...\n");

            if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
            {
                puts("Receiving failed.\n");
            }
            else
            {
                puts(server_reply);
                myUser = string(user);
            }
            bzero(server_reply, MAX_LENGTH);
        }
        else if (command == 3) // list
        {
            string msg = "List";
            send(sd, msg.c_str(), sizeof(msg), 0);
            printf("Wait for server to response...\n");
            if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
            {
                puts("Receiving failed.\n");
            }
            else
            {
                cout << endl
                     << "-----My Balance and User List-----" << endl;
                puts(server_reply);
            }
            bzero(server_reply, MAX_LENGTH);
        }
        else if (command == 4) // transaction
        {
            sending();
            // recv detection inside sending function
            bzero(server_reply, MAX_LENGTH);
        }
        else if (command == 5) // exit
        {
            string msg = "Exit";
            send(sd, msg.c_str(), sizeof(msg), 0);
            if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
            {
                puts("Receiving failed.\n");
            }
            else
            {
                puts(server_reply);
            }
            bzero(server_reply, MAX_LENGTH);
            close(sd);
            break;
        }
        else if (command == 6) // debug
        {
            printf("[Enter Message (limit 100)] > ");
            char message[100];
            scanf("%s", message);

            send(sd, message, sizeof(message), 0);
            printf("Wait for server to response...\n");

            if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
            {
                puts("Receiving failed.\n");
            }
            else
            {
                puts(server_reply);
            }
            bzero(server_reply, MAX_LENGTH);
        }
        else
        {
            printf("Try again!");
        }
        menu();
    };
    return 0;
}

void *receive_thread(void *server_fd)
{
    int s_fd = *((int *)server_fd);
    while (1)
    {
        sleep(2);
        receiving(s_fd);
    }
}

void receiving(int server_fd)
{
    char clientmsg[MAX_LENGTH]; // client's message
    int client_sd, valread;
    struct sockaddr_in client_;
    socklen_t addrlen = sizeof(client_);
    client_sd = accept(server_fd, (struct sockaddr *)&client_, &addrlen);

    valread = recv(client_sd, clientmsg, MAX_LENGTH, 0);

    send(sd, clientmsg, sizeof(clientmsg), 0);
    bzero(clientmsg, MAX_LENGTH);
    close(client_sd);

    return;
}

void sending()
{
    bzero(server_reply, MAX_LENGTH);
    send(sd, "List", sizeof("List"), 0);
    if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
    }
    else
    {
        puts(server_reply);
    }

    string list;
    list = server_reply;
    bzero(server_reply, MAX_LENGTH);

    cout << myUser << " is transfering to";

    string user = myUser;
    string payee;
    printf("[Reveiver's username] > ");
    cin >> payee;
    int n = list.find(payee);
    int pound2 = list.find("#", n + payee.length() + 1);

    string payee_ip = list.substr(n + payee.length() + 1, pound2 - (n + payee.length() + 1));
    cout << "payee ip = " << payee_ip << endl;
    string payee_port = list.substr(pound2 + 1, 4);
    // payee_port.pop_back();
    cout << "payee port = " << payee_port << endl;
    string amount;
    printf("[Transaction amount] > ");
    cin >> amount;

    string msg = user;
    msg.append("#");
    msg.append(amount);
    msg.append("#");
    msg.append(payee);

    char *payee_port_ = const_cast<char *>(payee_port.c_str());

    int client_sd;
    char client_reply[MAX_LENGTH];

    client_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sd == -1)
    {
        printf("Couldn't create socket.\n");
    }

    struct sockaddr_in client_send;
    // client_send.sin_addr.s_addr = inet_addr(payee_ip.c_str());
    client_send.sin_addr.s_addr = INADDR_ANY;
    client_send.sin_family = AF_INET;
    client_send.sin_port = htons(stoi(payee_port, nullptr, 10));

    if (connect(client_sd, (struct sockaddr *)&client_send, sizeof(client_send)) < 0)
    {
        printf("Connection error.\n");
    }
    else
    {
        printf("Connected!\n");
    }

    send(client_sd, msg.c_str(), sizeof(msg), 0);

    close(client_sd);

    return;
}

void menu()
{
    printf("\n==========MENU===========\n");
    printf("1 -> Register\n");
    printf("2 -> Login\n");
    printf("3 -> Show My Balance and User List\n");
    printf("4 -> Make a Transaction\n");
    printf("5 -> Exit\n");
    printf("6 -> Debug\n");
    printf("=========================\n\n");
}