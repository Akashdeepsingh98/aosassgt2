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
//void SendThread();
//void RecThrMan();
//void SendThrMan();

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
    string myport = myipport.substr(myipport.find(":"));
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
    //thread senthrman(SendThrMan);
    vector<thread> recvthrs;
    vector<thread> sendthrs;
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
            char temp[1024] = {'\0'};
            int n = write(server_sock_fd, ucom.c_str(), ucom.size());
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "login")
        {
            char temp[1024] = {'\0'};
            int n = write(server_sock_fd, ucom.c_str(), ucom.size());
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "create_group")
        {
            char temp[1024] = {'\0'};
            int n = write(server_sock_fd, ucom.c_str(), ucom.size());
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "download_file")
        {
            char temp[1024] = {'\0'};
            string group_id, filename, dest_path;
            ss >> group_id;
            int n;
            {
                string t = command + " " + group_id;
                n = write(server_sock_fd, t.c_str(), t.size());
            }
            n = read(server_sock_fd, temp, sizeof(temp) - 1);
            cout << temp << endl;

            string ip, port;
            {
                stringstream tempss(temp);
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
    sender_addr.sin_addr.s_addr = atoi(ip.c_str());
    sender_addr.sin_port = atoi(port.c_str());

    if (connect(sender_sockfd, (struct sockaddr *)&sender_addr, sizeof(sender_addr)) < 0)
    {
        return;
    }

    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));
    int n = write(sender_sockfd, filename.c_str(), filename.size());

    ofstream f(dest_path + filename);
    n = read(sender_sockfd, mainbuffer, sizeof(mainbuffer) - 1);

    while (string(mainbuffer) != ip + ":" + port)
    {
        f << mainbuffer;
        bzero(mainbuffer, sizeof(mainbuffer));
    }
    f.close();
}