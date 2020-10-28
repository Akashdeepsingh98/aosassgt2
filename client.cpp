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

    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(server_port);

    if (connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cout << "Cannot connect to tracker" << endl;
        exit(1);
    }

    string buffer = "create_user akash qwerty";
    int n = write(server_sock_fd, buffer.c_str(), sizeof(buffer));
    cout << n << endl;
    close(server_sock_fd);
}