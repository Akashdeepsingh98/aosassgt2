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
void ReceiveThread(string ip, string port, string filename, string dest_path, string myip, string myport);
void minRecv(string filename, string dest_path, string senderip, string senderport, int start, int end);
void SendThrMan(int mysockfd, string myip, string myport);
void SendThread(int client_sockfd, struct sockaddr_in client_addr, string myip, string myport);
map<string, vector<string>> avfiles;
map<string, string> folders;
map<string, vector<string>> mygroups;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Give ip:port and tracker_info" << endl;
        exit(1);
    }

    int trackerfd = socket(AF_INET, SOCK_STREAM, 0);
    int trackerport = 20001;
    string trackerip;

    {
        string t;
        ifstream trackerInfoFile(argv[1]);
        getline(trackerInfoFile, trackerip);
        getline(trackerInfoFile, t);
        trackerport = atoi(t.c_str());
    }

    struct hostent *trackeriph = gethostbyname("127.0.0.1");
    struct sockaddr_in trackeraddr;
    bzero((char *)&trackeraddr, sizeof(trackeraddr));
    trackeraddr.sin_family = AF_INET;
    bcopy((char *)trackeriph->h_addr, (char *)&trackeraddr.sin_addr.s_addr, trackeriph->h_length);
    trackeraddr.sin_port = htons(trackerport);
    if (connect(trackerfd, (struct sockaddr *)&trackeraddr, sizeof(trackeraddr)) < 0)
    {
        cout << "Cannot connect to tracker" << endl;
        exit(1);
    }
    // tracker connection ready
    cout << "works till here" << endl;
    int mysockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in myaddr;
    string myipport = argv[1];
    string myip = myipport.substr(0, myipport.find(":"));
    string myport = myipport.substr(myipport.find(":") + 1);
    bzero((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(myip.c_str())->h_addr, (char *)&myaddr.sin_addr.s_addr, gethostbyname(myip.c_str())->h_length);
    myaddr.sin_port = htons(atoi(myport.c_str()));
    if (bind(mysockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        error(string("Cannot bind my_sock_fd and my_addr"));
    }
    listen(mysockfd, 10);
    // my own socket
    cout << "works till here" << endl;

    thread senthrman(SendThrMan, mysockfd, myip, myport);
    string ucom = "";
    vector<thread> recvthrs;
    while (true)
    {
        getline(cin, ucom);
        stringstream ss;
        ss << ucom;
        string command;
        ss >> command;
        if (ucom == "logout")
        {
            write(trackerfd, ucom.c_str(), ucom.size());
            break;
        }
        else if (command == "create_user")
        {
            int n;
            char temp[1024];
            bzero(temp, sizeof(temp));
            {
                string t = ucom + " " + myip + " " + myport;
                n = write(trackerfd, t.c_str(), t.size());
            }

            n = read(trackerfd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "login")
        {
            int n;
            char temp[1024];
            bzero(temp, sizeof(temp));
            {
                string t = ucom + " " + myip + " " + myport;
                n = write(trackerfd, t.c_str(), t.size());
            }
            n = read(trackerfd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "create_group")
        {
            char temp[1024];
            bzero(temp, sizeof(temp));
            int n = write(trackerfd, ucom.c_str(), ucom.size());
            n = read(trackerfd, temp, sizeof(temp) - 1);
            cout << temp << endl;
        }
        else if (command == "download_file")
        {
            char temp[1024];
            bzero(temp, sizeof(temp));
            string group_id, filename, destpath;
            ss >> group_id >> filename >> destpath;
            string msg = command + " " + group_id;
            write(trackerfd, msg.c_str(), msg.size());
            read(trackerfd, temp, sizeof(temp) - 1);
            cout << temp << endl;

            string masterip, masterport;
            stringstream masteripport;
            masteripport << temp;
            masteripport >> masterip >> masterport;

            recvthrs.push_back(thread(ReceiveThread, masterip, masterport, filename, destpath, myip, myport));
        }
        else if (command == "upload_file")
        {
            string fn;
            ss >> fn;

            // make a directory to store chunks for each file
            // linearly name the directories starting from 0
            // use the index of a filename in uploadedfs to
            // determine folder name when reading
            struct stat info;
            if (mkdir(to_string(avfiles.size()).c_str(), 0777) == -1 && stat(to_string(avfiles.size()).c_str(), &info) == 0)
            {
                cout << "Cannot upload file" << endl;
            }
            else
            {
                avfiles[fn].push_back(myip + ":" + myport);
                folders[fn] = to_string(avfiles.size() - 1);
                ifstream mainfile;
                mainfile.open(fn, ios::in | ios::binary);
                if (mainfile.is_open())
                {
                    ofstream output;
                    int counter = 1;
                    string fullChunkName;
                    char chunkbf[512 * 1024 + 1];
                    bzero(chunkbf, sizeof(chunkbf));
                    while (!mainfile.eof())
                    {
                        fullChunkName = to_string(avfiles.size() - 1) + "/" + fn + "." + to_string(counter);
                        output.open(fullChunkName, ios::out | ios::trunc | ios::binary);
                        bzero(chunkbf, sizeof(chunkbf));
                        if (output.is_open())
                        {
                            mainfile.read(chunkbf, sizeof(chunkbf) - 1);
                            output << chunkbf;
                            output.close();
                            counter++;
                        }
                    }
                    mainfile.close();
                }
            }
        }
    }
}

void SendThrMan(int mysockfd, string myip, string myport)
{
    vector<thread> sendthrs;
    struct sockaddr_in client_addr;
    socklen_t client_length = sizeof(client_addr);
    vector<int> client_sock_fds;
    cout << "works till here" << endl;
    while (true)
    {
        bzero((char *)&client_addr, client_length);
        cout << "works till here" << endl;
        client_sock_fds.push_back(accept(mysockfd, (struct sockaddr *)&client_addr, &client_length));
        sendthrs.push_back(thread(SendThread, client_sock_fds.back(), client_addr, myip, myport));
    }
}

void SendThread(int client_sockfd, struct sockaddr_in client_addr, string myip, string myport)
{
    stringstream temp;
    auto myid = this_thread::get_id();
    temp << myid;
    ofstream senderlog("senderlog" + temp.str() + ".txt", ios::app);
    char mainbuffer[512 * 1024 + 1];
    bzero(mainbuffer, sizeof(mainbuffer));

    int n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
    senderlog << mainbuffer << endl;
    stringstream ss;
    ss << mainbuffer;
    bzero(mainbuffer, sizeof(mainbuffer));
    string filename;
    write(client_sockfd, "a", 1);
    string reqtype;
    ss >> reqtype;
    if (reqtype == "mainsender")
    {
        string partnerip, partnerport;
        ss >> partnerip >> partnerport;
        read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
        filename = mainbuffer;
        senderlog << mainbuffer << endl;
        bzero(mainbuffer, sizeof(mainbuffer));
        ifstream general;
        general.open(filename, ios::in | ios::binary);

        // only send if all chunks are present i.e. this peer is a seeder
        if (general.is_open())
        {
            DIR *dp;
            int chunkcount = 0;
            struct dirent *ep;
            dp = opendir(string(folders[filename] + "/").c_str());

            if (dp != NULL)
            {
                while (ep = readdir(dp))
                    chunkcount++;
                closedir(dp);
            }
            chunkcount -= 2;
            string data = to_string(chunkcount) + " ";
            for (int i = 0; i < avfiles[filename].size() - 1; i++)
            {
                data += avfiles[filename][i] + " ";
            }
            data += avfiles[filename].back();
            n = write(client_sockfd, data.c_str(), data.size());
            avfiles[filename].push_back(partnerip + ":" + partnerport);
            ofstream avlog("avlog.txt");
            avlog << filename << " ";
            for (int i = 0; i < avfiles[filename].size(); i++)
            {
                avlog << avfiles[filename][i] << " ";
            }
            avlog.close();
        }
        else
        {
            n = write(client_sockfd, string("stop").c_str(), string("stop").size());
            return;
        }
    }
    else
    {
        //n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
        //senderlog << mainbuffer << endl;
        //stringstream ss;
        //ss << mainbuffer;
        int lowerbound, upperbound;
        ss >> filename >> lowerbound >> upperbound;
        senderlog << "bounds: " << lowerbound << " " << upperbound << endl;
        bzero(mainbuffer, sizeof(mainbuffer));
        string chunkpref = folders[filename] + "/" + filename;
        for (int i = lowerbound; i <= upperbound; i++)
        {
            senderlog << i << endl;
            string chunkname = chunkpref + "." + to_string(i);
            senderlog << chunkname << endl;
            FILE *chunk = fopen(chunkname.c_str(), "r");
            char c;
            int j = 0;
            while ((c = fgetc(chunk)) != EOF)
            {
                mainbuffer[j++] = c;
            }
            n = write(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
            write(client_sockfd, "\0", 1);
            //senderlog << mainbuffer << endl;
            senderlog << n << endl;
            n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
            while (n <= 0)
            {
                n = read(client_sockfd, mainbuffer, sizeof(mainbuffer) - 1);
            }
            senderlog << mainbuffer << endl;
            bzero(mainbuffer, sizeof(mainbuffer));
        }
    }
    senderlog.close();
}

void ReceiveThread(string masterip, string masterport, string filename, string dest_path, string myip, string myport)
{
    int masterfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in masteraddr;
    bzero((char *)&masteraddr, sizeof(masteraddr));
    masteraddr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(masterip.c_str())->h_addr, (char *)&masteraddr.sin_addr.s_addr, gethostbyname(masterip.c_str())->h_length);
    masteraddr.sin_port = htons(atoi(masterport.c_str()));

    ofstream clientlog("clientlog.txt");

    if (connect(masterfd, (struct sockaddr *)&masteraddr, sizeof(masteraddr)) < 0)
    {
        clientlog << "cannot connect to master\n";
        return;
    }

    // tell whether to act as mainsender or minisender
    string msg = "mainsender " + myip + " " + myport;
    write(masterfd, msg.c_str(), msg.size());
    clientlog << msg << endl;
    char mainbuffer[512 * 1024 + 1];
    read(masterfd, mainbuffer, sizeof(mainbuffer) - 1);
    bzero(mainbuffer, sizeof(mainbuffer));

    // send filename
    int n = write(masterfd, filename.c_str(), filename.size());

    // get chunkcount
    n = read(masterfd, mainbuffer, sizeof(mainbuffer) - 1);
    clientlog << mainbuffer << endl;
    if (string(mainbuffer) == "stop")
    {
        return;
    }

    int chunkcount;
    stringstream ipsnportss;
    ipsnportss << mainbuffer;
    ipsnportss >> chunkcount;

    vector<string> ipportvec;
    while (!ipsnportss.eof())
    {
        string t;
        ipsnportss >> t;
        ipportvec.push_back(t);
    }

    mkdir(dest_path.c_str(), 0777);
    vector<thread> minRecThreads;
    int start = 1;
    for (int i = 0; i < ipportvec.size() - 1; i++)
    {
        string senderip, senderport;
        senderip = ipportvec[i].substr(0, ipportvec[i].find(":"));
        senderport = ipportvec[i].substr(ipportvec[i].find(":") + 1);
        minRecThreads.push_back(thread(minRecv, filename, dest_path, senderip, senderport, start, start - 1 + chunkcount / ipportvec.size()));
        start += chunkcount / ipportvec.size();
    }

    clientlog << ipportvec.back().substr(0, ipportvec.back().find(":")) << " " << ipportvec.back().substr(ipportvec.back().find(":") + 1) << start << " " << chunkcount << endl;

    minRecThreads.push_back(thread(minRecv, filename, dest_path, ipportvec.back().substr(0, ipportvec.back().find(":")), ipportvec.back().substr(ipportvec.back().find(":") + 1), start, chunkcount));
    for (int i = 0; i < minRecThreads.size(); i++)
    {
        minRecThreads[i].join();
    }
    folders[filename] = dest_path;

    {
        string chunkfilename;
        ofstream outputfile;
        outputfile.open(dest_path + "/" + filename, ios::out | ios::binary);
        if (outputfile.is_open())
        {
            bool filefound = true;
            int counter = 1;
            int filesize = 0;
            while (filefound)
            {
                filefound = false;
                chunkfilename = dest_path + "/" + filename + "." + to_string(counter);
                ifstream chunkinput;
                chunkinput.open(chunkfilename, ios::in | ios::binary);

                if (chunkinput.is_open())
                {
                    filefound = true;
                    filesize = 512 * 1024;
                    char inputbuf[filesize];
                    bzero(inputbuf, sizeof(inputbuf));
                    chunkinput >> inputbuf;
                    outputfile << inputbuf;
                    chunkinput.close();
                    remove(chunkfilename.c_str());
                }
                counter++;
            }
            outputfile.close();
        }
    }

    clientlog.close();
}

void minRecv(string filename, string dest_path, string senderip, string senderport, int start, int end)
{
    stringstream temp;
    auto myid = this_thread::get_id();
    temp << myid;
    ofstream clientlog("clientlog" + temp.str() + ".txt", ios::app);
    int senderfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in senderaddr;
    bzero((char *)&senderaddr, sizeof(senderaddr));
    senderaddr.sin_family = AF_INET;
    bcopy((char *)gethostbyname(senderip.c_str())->h_addr, (char *)&senderaddr.sin_addr.s_addr, gethostbyname(senderip.c_str())->h_length);
    senderaddr.sin_port = htons(atoi(senderport.c_str()));

    if (connect(senderfd, (struct sockaddr *)&senderaddr, sizeof(senderaddr)) < 0)
    {
        clientlog << "cannot connect to master\n";
        return;
    }
    clientlog << start << " " << end << " " << endl;
    string msg = "minisender " + filename + " " + to_string(start) + " " + to_string(end);
    write(senderfd, msg.c_str(), msg.size());

    char mainbuffer[512 * 1024 + 1];
    read(senderfd, mainbuffer, sizeof(mainbuffer) - 1);
    clientlog << mainbuffer << endl;
    bzero(mainbuffer, sizeof(mainbuffer));
    ofstream f;
    for (int i = start; i <= end; i++)
    {
        clientlog << i << endl;
        f.open(dest_path + "/" + filename + "." + to_string(i), ios::out | ios::trunc | ios::binary);
        int n;
        bool flag = true;
        while (flag)
        {
            bzero(mainbuffer, sizeof(mainbuffer)); //clear the variable
            n = read(senderfd, mainbuffer, 512);
            clientlog << n << endl;
            if (n <= 1 && mainbuffer[0] == '\0')
            {
                clientlog << "File " << i << " completed" << endl;
                flag = false;
                f.close();
                string t = "a";
                write(senderfd, t.c_str(), t.size());
            }
            else
            {
                //total_size += size_recv;
                f << mainbuffer;
            }
        }
    }
    clientlog.close();
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