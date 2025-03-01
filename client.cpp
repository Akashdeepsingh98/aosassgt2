#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

void Server_Threads();

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    Server_Threads();
    //printf("Please enter the message: ");
    //bzero(buffer, 256);
    //while (true)
    //{
    //    fgets(buffer, 255, stdin);
    //    n = write(sockfd, buffer, strlen(buffer));
    //    if (n < 0)
    //        error("ERROR writing to socket");
    //    if (string(buffer).substr(0, 4) == string("quit"))
    //        break;
    //    bzero(buffer, 256);
    //}
    //n = read(sockfd, buffer, 255);
    //if (n < 0)
    //    error("ERROR reading from socket");
    //printf("%s\n", buffer);
    close(sockfd);
    return 0;
}

void Server_Threads()
{
    int portno = 30001;
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    int clientsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    char buffer[512 * 1024];
    ifstream f("trial.txt");
    bzero(buffer, sizeof(buffer));
    
    while (!f.eof())
    {
        f.read(buffer, sizeof(buffer));
        //cout << buffer << endl;
        int n = write(clientsockfd, buffer, strlen(buffer));
        bzero(buffer, sizeof(buffer));
    }
    f.close();
    close(sockfd);
}