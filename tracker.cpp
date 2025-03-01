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
#include <arpa/inet.h>
using namespace std;

void ThreadForClient(int sockfd);
void ThreadManager(int sockfd, struct sockaddr_in cli_addr, socklen_t clilen);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // read tracker info
    ifstream trackerInfoFile(argv[1]);
    int trackerno = atoi(argv[2]);

    string tracker_ip_addr;
    string port_str;

    for(int i=0;i<trackerno;i++)
    {
        getline(trackerInfoFile, tracker_ip_addr);
        getline(trackerInfoFile, port_str);
    }
    int portno = atoi(port_str.c_str());
    trackerInfoFile.close();

    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    //int n;
    if (argc < 3)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    thread threadMan(ThreadManager, sockfd, cli_addr, clilen);
    string qcom;
    while (true)
    {
        getline(cin, qcom);
        if (qcom == string("quit"))
        {
            break;
        }
    }

    close(sockfd);
    return 0;
}

void ThreadForClient(int sockfd)
{
    thread::id this_id = this_thread::get_id();
    ofstream outfile;
    outfile.open(string("output" + to_string(sockfd) + ".txt").c_str());
    char buffer[256];

    while (true)
    {
        bzero(buffer, 256);
        int n = 0;
        while (n <= 0)
        {
            if (write(sockfd, buffer, 255) == -1)
            {
                outfile << this_id << endl;
                outfile.close();
                close(sockfd);
                return;
            }
            n = read(sockfd, buffer, 255);
        }
        outfile << buffer << endl;
    }
}

void ThreadManager(int sockfd, struct sockaddr_in cli_addr, socklen_t clilen)
{
    vector<int> sockfds;
    vector<thread> allthreads;
    while (true)
    {
        sockfds.push_back(accept(sockfd, (struct sockaddr *)&cli_addr, &clilen));
        if (sockfds.back() < 0)
            error("ERROR on accept");

        printf("server: got connection from %s port %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        thread thread_obj(ThreadForClient, sockfds.back());
        allthreads.push_back(move(thread_obj));
    }
}