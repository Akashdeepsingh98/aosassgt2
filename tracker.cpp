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

int main(int argc, char *argv[])
{
    // read tracker info
    ifstream trackerInfoFile(argv[1]);
    int trackerno = atoi(argv[2]);

    string tracker_ip_addr;
    string port_str;

    for (int i = 0; i < trackerno; i++)
    {
        getline(trackerInfoFile, tracker_ip_addr);
        getline(trackerInfoFile, port_str);
    }
    int portno = atoi(port_str.c_str());
    trackerInfoFile.close();
}