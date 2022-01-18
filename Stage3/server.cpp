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

#include <math.h> //ENCR: for encryption math

using namespace std;

#define MAX_LENGTH 2000

void *connection_handler(void *);
void regist(int sock, char *cmd);
void login(int sock, char *client_message);
void trans(int sock, char *client_message);
void debug(int sock, char *client_message);

//ENCR: variables for encryption
int n, t, flag;
long int e[500], d[500], j; // server global

void encryption_key(int x, int y);         //ENCR: generate encryption key
int prime(long int);                       //ENCR: function to check for prime number
long int cd(long int);                     //ENCR: child function of encryption_key()
string decrypt(char *client_encr_message); //ENCR: function to decrypt

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

    // ENCR: TODO: variables
    int x = 17;
    int y = 31;

    //ENCR: create public key
    encryption_key(x, y);
    cout << "Public key generated: " << n << "-" << e[0] << endl;

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
    char client_message[MAX_LENGTH];
    string message;
    bool encr = false;

    //Send some messages to the client
    puts("Connection handler assigned.");

    //Receive a message from client
    while ((read_size = recv(sock, client_message, MAX_LENGTH, 0)) > 0)
    {
        if (encr)
        {
            //ENCR:
            cout << "Received " << client_message << endl;
            string de_msg = decrypt(client_message);
            cout << endl
                 << "decrypted message: " << de_msg << endl;

            char _de_msg[de_msg.length()];
            strcpy(_de_msg, de_msg.c_str());

            trans(sock, _de_msg);
            encr = false;
            continue;
        }
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
        else if (strcmp(client_message, "Encrypted") == 0)
        {
            puts("Username not found");
            message = "ENCRYPTION_REQ_ACCEPT\n";
            write(sock, message.c_str(), sizeof(message));
            encr = true;
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
                puts("Bad input");
                message = "Bad input";
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
    char callback[MAX_LENGTH] = "Login > ";
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
    char callback[MAX_LENGTH] = "Transaction > from:";
    char usernameA[100];
    char usernameB[100];
    char amount[100];

    sscanf(client_message, "%[^#]#%[^#]#%[^\n]", usernameA, amount, usernameB);
    strcat(callback, usernameA);
    strcat(callback, " to: ");
    strcat(callback, usernameB);
    strcat(callback, " amount= ");
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

    puts("server recorded transaction!");
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
    message.append(to_string(n));
    message.append("-");
    message.append(to_string(e[0]));
    message.append("\n"); // DONE: server public key
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

    write(sock, message.c_str(), MAX_LENGTH);
}

//TODO:
int duplicatePort(struct User *p, int port) {}

// ----------------------------------------------------------------
// ENCR: functions belows are for encryption

//function to generate encryption key
void encryption_key(int x, int y)
{
    n = x * y;
    t = (x - 1) * (y - 1);

    int k = 0;
    int flag;
    for (int i = 2; i < t; i++)
    {
        if (t % i == 0)
            continue;
        flag = prime(i);
        if (flag == 1 && i != x && i != y)
        {
            e[k] = i;
            flag = cd(e[k]);
            if (flag > 0)
            {
                d[k] = flag;
                k++;
            }
            if (k == 99)
                break;
        }
    }
}

int prime(long int pr)
{
    int i;
    j = sqrt(pr);
    for (i = 2; i <= j; i++)
    {
        if (pr % i == 0)
            return 0;
    }
    return 1;
}

long int cd(long int a)
{
    long int k = 1;
    while (1)
    {
        k = k + t;
        if (k % a == 0)
            return (k / a);
    }
}

// TODO: temp -> encrypted message code input from client

string decrypt(char *client_encr_message)
{
    int len = 0;
    long int pt, ct, key = d[0], k;

    cout << "client message: " << client_encr_message << endl;

    int temp[MAX_LENGTH];
    int temp_idx = 0;
    string tempss = "";
    for (int i = 0; i <= strlen(client_encr_message); i++)
    {
        if (client_encr_message[i] == '-' || client_encr_message[i] == '\0')
        {
            // cout << tempss << endl;
            temp[temp_idx] = stoi(tempss);
            tempss = "";
            temp_idx++;
            len++;
        }
        else
        {
            tempss += client_encr_message[i];
        }
    }

    char de[MAX_LENGTH]; // displaying decrypted message
    string decrypted = "";

    int i = 0;
    while (i < len)
    {
        ct = temp[i];
        k = 1;
        for (j = 0; j < key; j++)
        {
            k = k * ct;
            k = k % n;
        }
        pt = k + 96;
        de[i] = pt;
        decrypted += de[i];
        i++;
    }

    int h1 = decrypted.find("~", 0);
    int h2 = decrypted.find("~", h1 + 1);

    string payer = decrypted.substr(0, h1);
    string amount = decrypted.substr(h1 + 1, h2 - h1 - 1);
    string payee = decrypted.substr(h2 + 1, len - h2 - 1);
    string real_amount;

    // amount translate
    for (int a = 0; a < amount.length(); a++)
    {
        int ra = amount.c_str()[a];
        ra -= 97;
        real_amount += to_string(ra);
    }

    cout << "decryption details" << endl
         << "payer: " << payer << " amount: " << amount << " real_amount: " << real_amount << " payee: " << payee << endl;

    string re_de = "";
    re_de.append(payer);
    re_de.append("#");
    re_de.append(real_amount);
    re_de.append("#");
    re_de.append(payee);

    return re_de;
}
