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

#include <math.h> //ENCR: for encryption math

using namespace std;

#define MAX_LENGTH 2000

void *receive_thread(void *sd);
void receiving(int server_sd);
void sending();
void menu();
char server_reply[MAX_LENGTH];
int sd;

string myUser;

//ENCR: variables for encryption
int n, t, flag;
long int e[500], d[500], j; // server global

void encryption_key(int x, int y); //ENCR: generate encryption key
int prime(long int);               //ENCR: function to check for prime number
long int cd(long int);             //ENCR: child function of encryption_key()
string encrypt(int n, int e0, char *message, int len);
string decrypt(char *message); //ENCR: function to decrypt

void getServerKey(char *server_reply);

int server_n;
int server_e0;

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
    string buffer;

    // cout << "call menu from init" << endl;
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
                getServerKey(server_reply);
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
        // cout << "summon menu via command " << command << endl;
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
    int client_sd;
    struct sockaddr_in client_;
    socklen_t addrlen = sizeof(client_);
    client_sd = accept(server_fd, (struct sockaddr *)&client_, &addrlen);

    // get message from peer client -> should be "PeerEncrypted"
    string kr = "";
    if (recv(client_sd, clientmsg, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
        return;
    }
    else
    {
        if (strcmp(clientmsg, "PeerEncrypted") == 0)
        {
            encryption_key(5, 7); // ENCR: input two primes TODO: variables
            kr.append(to_string(n));
            kr.append("-");
            kr.append(to_string(e[0]));
            write(client_sd, kr.c_str(), sizeof(kr));
        }
        else
        {
            return;
        }
    }
    bzero(clientmsg, MAX_LENGTH);

    puts("Wait for peer client to transfer (by sending encrypted message)...\n");
    char server_notice[MAX_LENGTH];
    if (recv(client_sd, clientmsg, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
        return;
    }
    else
    {
        string ss_de_clientmsg = decrypt(clientmsg);
        int h1 = ss_de_clientmsg.find("~");
        int h2 = ss_de_clientmsg.find("~", h1 + 1);

        string amount = ss_de_clientmsg.substr(h1 + 1, h2 - h1 - 1);

        string ss_sn = "";
        string en_amount = "";

        int itmp;
        char ctmp;
        for (int z = 0; z < amount.length(); z++)
        {
            itmp = amount.c_str()[z];
            // cout << itmp << " ";
            itmp += 49;
            ctmp = itmp;
            // cout << ctmp << endl;
            en_amount += ctmp;
        }

        string payer = ss_de_clientmsg.substr(0, h1);
        string payee = ss_de_clientmsg.substr(h2 + 1, ss_de_clientmsg.length() - h2 - 1);

        ss_sn.append(payer);
        ss_sn.append("~");
        ss_sn.append(en_amount);
        ss_sn.append("~");
        ss_sn.append(payee);

        // cout << "ss_sn" << ss_sn << endl;

        strcpy(server_notice, ss_sn.c_str());
    }

    //ENCR: encrypt this message with server's public key and send to the server
    string encr_req = "Encrypted";
    string encr_code;
    send(sd, encr_req.c_str(), sizeof(encr_req), 0);
    printf("Wait for server to response...\n");

    if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
    }
    else
    {
        puts(server_reply);
        encr_code = encrypt(server_n, server_e0, server_notice, sizeof(server_notice));
        send(sd, encr_code.c_str(), encr_code.length(), 0);
        bzero(server_notice, MAX_LENGTH);
    }
    bzero(server_reply, MAX_LENGTH);

    close(client_sd);

    return;
}

void sending()
{
    bzero(server_reply, MAX_LENGTH);
    send(sd, "List", sizeof("List"), 0);

    // cout << "| sent List to server" << endl;

    if (recv(sd, server_reply, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
    }
    else
    {
        // cout << "--- got server reply" << endl;
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

    // string payee_ip = list.substr(n + payee.length() + 1, pound2 - (n + payee.length() + 1));
    // cout << "payee ip = " << payee_ip << endl;
    string payee_port = list.substr(pound2 + 1, 4);
    // payee_port.pop_back();
    cout << "payee port = " << payee_port << endl;
    string amount;
    printf("[Transaction amount] > ");
    cin >> amount;

    //ENCR: amount number translate to letters
    string en_amount = "";
    int itmp;
    char ctmp;
    for (int z = 0; z < amount.length(); z++)
    {
        itmp = amount.c_str()[z];
        // cout << itmp << " ";
        itmp += 49;
        ctmp = itmp;
        // cout << ctmp << endl;
        en_amount += ctmp;
    }

    string msg = user;
    msg.append("~");
    msg.append(en_amount);
    msg.append("~");
    msg.append(payee);

    // cout << "en_amount: " << en_amount << endl;

    char *payee_port_ = const_cast<char *>(payee_port.c_str());

    int client_sd;
    char client_reply[MAX_LENGTH];

    client_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sd == -1)
    {
        printf("Couldn't create socket.\n");
    }
    cout << "### Peer client socket created" << endl;

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
        printf("### Peer client connected!\n");
    }

    //ENCR: send request to peer client
    string encr_req = "PeerEncrypted";
    string encr_code;
    send(client_sd, encr_req.c_str(), sizeof(encr_req), 0);
    printf("Peer public key requested, Wait for peer client to response with the key...\n");

    if (recv(client_sd, client_reply, MAX_LENGTH, 0) < 0)
    {
        puts("Receiving failed.\n");
    }
    else
    {
        //ENCR: handle peer public key
        cout << endl
             << "(peer client reponse)" << endl;
        puts(client_reply);
        string client_reply_ss(client_reply);
        string cn = client_reply_ss.substr(0, client_reply_ss.find('-'));
        string ce = client_reply_ss.substr(client_reply_ss.find('-') + 1, client_reply_ss.length() - client_reply_ss.find('-') - 1);
        int client_n = stoi(cn);
        int client_e0 = stoi(ce);

        // cout << endl
        //     << "peer public key n = " << client_n << " client_e0 = " << client_e0 << endl;

        //ENCR: handle encryption
        char _msg[msg.length()];
        strcpy(_msg, msg.c_str());
        encr_code = encrypt(client_n, client_e0, _msg, sizeof(_msg));

        cout << "| message encrypted" << endl;

        //send encrypted message
        send(client_sd, encr_code.c_str(), encr_code.length(), 0);
        cout << "| encrypted message sent" << endl;

        bzero(_msg, MAX_LENGTH);
    }

    close(client_sd);
    cout << endl
         << "return" << endl;

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
    printf("=========================\n\n:");
}

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

void getServerKey(char *server_reply)
{
    string sr = server_reply;
    // cout << "---getServerKey---" << endl;
    int br1 = sr.find("\n", 0);
    int dash = sr.find("-", br1 + 1);
    int br2 = sr.find("\n", dash + 1);
    string n = sr.substr(br1 + 1, dash - br1 - 1);
    string e0 = sr.substr(dash + 1, br2 - dash - 1);
    server_n = stoi(n);
    server_e0 = stoi(e0);
    // cout << server_n << "-" << server_e0 << endl;
}

//ENCR: encryption
string encrypt(int n, int e0, char *message, int len)
{
    long int temp[MAX_LENGTH];
    char en[MAX_LENGTH];
    long int pt, ct, key = e0, k;
    int i = 0; // loop index
    //long int len = strlen(msg); // message length
    string en_code = "";

    cout << "n=" << n << ", e0=" << e0 << endl;
    //char m[MAX_LENGTH];
    //cin >> m;

    while (i != strlen(message))
    {
        pt = message[i]; // message
        pt = pt - 96;
        k = 1;
        for (int j = 0; j < key; j++)
        {
            k = k * pt;
            k = k % n;
        }
        temp[i] = k;
        ct = k + 96;
        en[i] = ct;

        if (i != 0)
        {
            en_code.append("-");
        }
        en_code.append(to_string(temp[i]));

        i++;
    }
    en[i] = -1;
    cout << "\n\nTHE ENCRYPTED MESSAGE / CODE IS\n";
    for (int j = 0; en[j] != -1; j++)
        cout << en[j];

    cout << endl
         << en_code << endl;

    return en_code;
}

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

    cout << "****** decryption details ******" << endl
         << "payer: " << payer << " amount: " << amount << " real_amount: " << real_amount << " payee: " << payee << endl;

    string re_de = "";
    re_de.append(payer);
    re_de.append("~");
    re_de.append(real_amount);
    re_de.append("~");
    re_de.append(payee);

    // cout << "original decrypted result: " << re_de << endl;

    return re_de;
}
