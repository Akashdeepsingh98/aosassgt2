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

void ReceiveThread(string ip, string port, string filename, string dest_path);
void SendThread(int client_sockfd, struct sockaddr_in client_addr, string myip, string myport);
//void RecThrMan();
void SendThrMan(int mysockfd, string myip, string myport);
void error(string msg);
void error(char *msg);

vector<string> uploadedfs;

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
        else if (command == "upload_file")
        {
            ofstream senderlog;
            string fn; // the file to upload
            ss >> fn;
            // make a directory to store chunks for each file
            // linearly name the directories starting from 0
            // use the index of a filename in uploadedfs to
            // determine folder name when reading
            struct stat info;
            if (mkdir(to_string(uploadedfs.size()).c_str(), 0777) == -1 && stat(to_string(uploadedfs.size()).c_str(), &info) == 0)
            {
                senderlog.open("senderlog.txt");
                senderlog << "cannot make directory\n";
                senderlog.close();
            }
            else
            {
                uploadedfs.push_back(fn);
                //read from this file
                ifstream mainfile;
                mainfile.open(fn, ios::in | ios::binary);
                if (mainfile.is_open())
                {
                    // an ofstream used by each chunk
                    ofstream output;
                    // count current chunk number
                    int counter = 1;
                    string fullChunkName;

                    // chunks will temporarily be stored in this buffer
                    char chunkbf[512 * 1024];
                    bzero(chunkbf, sizeof(chunkbf));

                    while (!mainfile.eof())
                    {
                        // each chunk name starts with filename and append a number at the end after a dot
                        fullChunkName = to_string(uploadedfs.size() - 1) + "/" + fn + "." + to_string(counter);
                        output.open(fullChunkName, ios::out | ios::trunc | ios::binary);
                        bzero(chunkbf, sizeof(chunkbf));
                        if (output.is_open())
                        {
                            mainfile.read(chunkbf, sizeof(chunkbf));
                            output << chunkbf;
                            output.close();
                            counter++;
                        }
                        else
                        {
                            senderlog.open("senderlog.txt");
                            senderlog << "cannot open files for chunks\n";
                            senderlog.close();
                        }
                    }

                    mainfile.close();
                }
                else
                {
                    senderlog.open("senderlog.txt", ios::app);
                    senderlog << "cannot open file to read\n";
                    senderlog.close();
                }
            }
        }
    }

    close(server_sock_fd);
}

void ReceiveThread(string ip, string port, string filename, string dest_path)
{
    // prepare connection to sender peer
    int sender_sockfd;
    struct sockaddr_in sender_addr;
    sender_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&sender_addr, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(ip.c_str())->h_addr, (char *)&sender_addr.sin_addr.s_addr, gethostbyname(ip.c_str())->h_length);
    sender_addr.sin_port = htons(atoi(port.c_str()));

    ofstream clientlog("clientlog.txt", ios_base::app);

    if (connect(sender_sockfd, (struct sockaddr *)&sender_addr, sizeof(sender_addr)) < 0)
    {
        clientlog << "cannot connect\n";
        return;
    }
    // ready to go

    // the main buffer
    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));
    // give the filename we want
    int n = write(sender_sockfd, filename.c_str(), filename.size());

    // let peer tell about whether it has entire file or not
    n = read(sender_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    clientlog << mainbuffer << endl;
    if (string(mainbuffer) == "stop")
    {
        return;
    }

    int chunkcount;
    {
        stringstream ss;
        ss << mainbuffer;
        ss >> chunkcount;
    }

    {
        string t = "1 " + to_string(chunkcount);
        n = write(sender_sockfd, t.c_str(), t.size());
    }

    clientlog << dest_path << endl;
    mkdir(dest_path.c_str(), 0777);
    bzero(mainbuffer, sizeof(mainbuffer));
    for (int i = 1; i <= chunkcount; i++)
    {
        ofstream output;
        string fullchunkname = dest_path + "/" + filename + "." + to_string(i);
        clientlog << fullchunkname << endl;
        output.open(fullchunkname, ios::out | ios::trunc | ios::binary);
        n = read(sender_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
        output << mainbuffer;
        bzero(mainbuffer, sizeof(mainbuffer));
        output.close();
    }
    clientlog.close();
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
    ofstream senderlog("senderlog.txt", ios::app);
    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));

    // get name of file peer wants
    int n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    string filename = mainbuffer;
    bzero(mainbuffer, sizeof(mainbuffer));

    // this ifstream will be used by every chunk
    ifstream general;
    general.open(filename, ios::in | ios::binary);

    // only send if all chunks are present i.e. this peer is a seeder
    if (general.is_open())
    {
        senderlog << filename << endl;
        int j;
        for (j = 0; j < uploadedfs.size(); j++)
        {
            if (uploadedfs[j] == filename)
            {
                break;
            }
        }
        DIR *dp;
        int chunkcount = 0;
        struct dirent *ep;
        dp = opendir(string(to_string(j) + "/").c_str());

        if (dp != NULL)
        {
            while (ep = readdir(dp))
                chunkcount++;
            closedir(dp);
        }
        chunkcount -= 2;
        n = write(client_sockfd, to_string(chunkcount).c_str(), to_string(chunkcount).size());
        senderlog << chunkcount << endl;
    }
    else
    {
        n = write(client_sockfd, string("stop").c_str(), string("stop").size());
        return;
    }

    // get the upper and lower bounds of the chunks to read
    n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    int lowerbound, upperbound;
    {
        stringstream ss;
        ss << mainbuffer;
        ss >> lowerbound >> upperbound;
    }
    bzero(mainbuffer, sizeof(mainbuffer));
    senderlog << lowerbound << " " << upperbound << endl;
    // get the folder where the files are
    int j;
    for (j = 0; j < uploadedfs.size(); j++)
    {
        if (uploadedfs[j] == filename)
        {
            break;
        }
    }
    string chunkpref = to_string(j) + "/" + filename;

    for (int i = lowerbound; i <= upperbound; i++)
    {
        string chunkname = chunkpref + "." + to_string(i);
        FILE *chunk = fopen(chunkname.c_str(), "r");
        if (chunk == NULL)
        {
            senderlog << "cannot open file" << endl;
        }
        else
        {
            senderlog << "opened file" << endl;
        }
        char c;
        int j = 0;
        while ((c = fgetc(chunk)) != EOF)
        {
            mainbuffer[j++] = c;
        }
        n = write(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
        senderlog << mainbuffer << endl;
        bzero(mainbuffer, sizeof(mainbuffer));
    }
    senderlog.close();
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