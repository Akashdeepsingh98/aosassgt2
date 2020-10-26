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
//void checkSockets(vector<thread> &allthreads, vector<int> &sockfds);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr = INADDR_ANY;

    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

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
    close(sockfd);
    return 0;
}

void ThreadForClient(int sockfd)
{
    thread::id this_id = this_thread::get_id();
    ofstream outfile;
    outfile.open(string("output" + to_string(sockfd) + ".txt").c_str());
    char buffer[256];
    send(sockfd, "Hello, world!\n", 13, 0);
    char err[1024];
    socklen_t errsize = 1024;
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
