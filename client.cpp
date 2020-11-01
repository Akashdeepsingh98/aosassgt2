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
#include <bits/stdc++.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sstream>
#include <dirent.h>
using namespace std;

void error(string msg);
void error(char *msg);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Give ip:port and tracker_info" << endl;
        exit(1);
    }

    int trackerfd = socket(AF_INET, SOCK_STREAM, 0);
    int trackerport = 20001;
    struct hostent *trackerip = gethostbyname("127.0.0.1");
    struct sockaddr_in trackeraddr;
    bzero((char *)&trackerip, sizeof(trackerip));
    trackeraddr.sin_family = AF_INET;
    bcopy((char *)trackerip->h_addr, (char *)&trackeraddr.sin_addr.s_addr, trackerip->h_length);
    trackeraddr.sin_port = htons(trackerport);
    if (connect(trackerfd, (struct sockaddr *)&trackeraddr, sizeof(trackeraddr)) < 0)
    {
        cout << "Cannot connect to tracker" << endl;
        exit(1);
    }

    int mysockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in myip;
    {

    }
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