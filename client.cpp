#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <thread>
#include <fstream>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>
using namespace std;

void ReceiveThread(string ip, string port, string filename, string dest_path);
void SendThread(int client_sockfd, struct sockaddr_in client_addr, string myip, string myport);
//void RecThrMan();
void SendThrMan(int mysockfd, string myip, string myport);
void error(string msg);
void error(char *msg);

int main(int argc, char *argv[])
{
    int server_sock_fd, server_port = 20001;
    struct hostent *server = gethostbyname("127.0.0.1");
    struct sockaddr_in server_addr;

    if (argc != 3)
    {
        cout << "Give 3 arguments" << endl;
        exit(1);
    }

    string myipport = argv[1];
    string myip = myipport.substr(0, myipport.find(":"));
    string myport = myipport.substr(myipport.find(":") + 1);
    int mysockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in myaddr;
    bzero((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(myip.c_str())->h_addr, (char *)&myaddr.sin_addr.s_addr, gethostbyname(myip.c_str())->h_length);
    myaddr.sin_port = htons(atoi(myport.c_str()));

    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(server_port);

    if (bind(mysockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        error(string("Cannot bind my_sock_fd and my_addr"));
    }

    listen(mysockfd, 10);

    if (connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cout << "Cannot connect to tracker" << endl;
        exit(1);
    }

    //thread recthrman(RecThrMan);
    thread senthrman(SendThrMan, mysockfd, myip, myport);
    vector<thread> recvthrs;
    string ucom = ""; //user command
    while (true)
    {
        getline(cin, ucom);
        stringstream ss(ucom);
        string command;
        ss >> command;
        if (ucom == "logout")
        {
            write(server_sock_fd, ucom.c_str(), ucom.size());
            break;
        }
        else if (command == "create_user")
        {
            int n;
            char temp[1024];
            bzero(temp, sizeof(temp));
            {
                string t = ucom + " " + myip + " " + myport;
                n = write(server_sock_fd, t.c_str(), t.size());
            }

            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "login")
        {
            int n;
            char temp[1024];
            bzero(temp, sizeof(temp));
            {
                string t = ucom + " " + myip + " " + myport;
                n = write(server_sock_fd, t.c_str(), t.size());
            }
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "create_group")
        {
            char temp[1024];
            bzero(temp, sizeof(temp));
            int n = write(server_sock_fd, ucom.c_str(), ucom.size());
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "download_file")
        {
            char temp[1024];
            bzero(temp, sizeof(temp));
            string group_id, filename, dest_path;
            ss >> group_id >> filename >> dest_path;
            int n;
            {
                string t = command + " " + group_id;
                n = write(server_sock_fd, t.c_str(), t.size());
            }
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;

            string ip, port;
            {
                string t = temp;
                stringstream tempss(t);
                tempss >> ip >> port;
            }

            recvthrs.push_back(thread(ReceiveThread, ip, port, filename, dest_path));
        }
    }

    close(server_sock_fd);
}

void ReceiveThread(string ip, string port, string filename, string dest_path)
{
    int sender_sockfd;
    struct sockaddr_in sender_addr;
    sender_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&sender_addr, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(ip.c_str())->h_addr, (char *)&sender_addr.sin_addr.s_addr, gethostbyname(ip.c_str())->h_length);
    sender_addr.sin_port = htons(atoi(port.c_str()));
    ofstream clientlog("clientlog.txt", ios_base::app);
    clientlog.close();
    if (connect(sender_sockfd, (struct sockaddr *)&sender_addr, sizeof(sender_addr)) < 0)
    {
        clientlog.open("clientlog.txt", ios_base::app);
        clientlog << "cannot connect\n";
        clientlog.close();
        return;
    }

    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));
    int n = write(sender_sockfd, filename.c_str(), filename.size());

    ofstream f(dest_path);
    n = read(sender_sockfd, mainbuffer, sizeof(mainbuffer) - 1);

    while (string(mainbuffer) != ip + ":" + port)
    {
        f << mainbuffer;
        bzero(mainbuffer, sizeof(mainbuffer));
        n = read(sender_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    }
    f << mainbuffer;
    f.close();
}

void SendThrMan(int mysockfd, string myip, string myport)
{
    vector<thread> sendthrs;
    struct sockaddr_in client_addr;
    socklen_t client_length = sizeof(client_addr);
    vector<int> client_sock_fds;
    while (true)
    {
        bzero((char *)&client_addr, client_length);
        client_sock_fds.push_back(accept(mysockfd, (struct sockaddr *)&client_addr, &client_length));
        sendthrs.push_back(thread(SendThread, client_sock_fds.back(), client_addr, myip, myport));
    }
}

void SendThread(int client_sockfd, struct sockaddr_in client_addr, string myip, string myport)
{
    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));
    int n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    string filename = mainbuffer;
    bzero(mainbuffer, sizeof(mainbuffer));

    ofstream senderlog("senderlog.txt", ios_base::app);
    senderlog << filename << endl;
    senderlog.close();

    FILE *file = fopen(filename.c_str(), "r");
    if (file == NULL)
    {
        ofstream senderlog("senderlog.txt", ios_base::app);
        senderlog << "cannot open file" << endl;
        senderlog.close();
    }
    else
    {
        ofstream senderlog("senderlog.txt", ios_base::app);
        senderlog << "opened file" << endl;
        senderlog.close();
    }
    senderlog.open("senderlog.txt", ios_base::app);
    char c;
    int i = 0;
    while ((c = fgetc(file)) != EOF)
    {
        senderlog << i << " ";
        mainbuffer[i++] = c;
        if (i >= 512 * 1024)
        {
            i = 0;
            senderlog << mainbuffer[0] << endl;
            n = write(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
            bzero(mainbuffer, sizeof(mainbuffer));
        }
    }
    n = write(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    bzero(mainbuffer, sizeof(mainbuffer));
    senderlog.close();
    //ifstream f(filename);
    //while (f.read(mainbuffer, sizeof(mainbuffer) - 1))
    //{
    //    ofstream clientlog("senderlog.txt", ios_base::app);
    //    clientlog << mainbuffer << endl;
    //    clientlog.close();
    //    n = write(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    //    bzero(mainbuffer, sizeof(mainbuffer));
    //}
    {
        string t = myip + ":" + myport;
        n = write(client_sockfd, t.c_str(), t.size());
    }
    //f.close();
}

void error(char *msg)
{
    cout << "Error: " << msg << endl;
    exit(1);
}

void error(string msg)
{
    cout << "Error: " << msg << endl;
    exit(1);
}