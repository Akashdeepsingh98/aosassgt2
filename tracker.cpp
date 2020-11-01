#include <map>
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
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <netdb.h>
using namespace std;

typedef struct
{
    string user_id;
    string passwd;
    string ipaddress;
    string portno;
    vector<string> groups;
} Client;

void error(char *msg);
void error(string msg);
void Client_Thread_Manager(int my_sock_fd, int mytrackerno);
void ClientThread(int client_sock_fd, struct sockaddr_in client_addr, int myno);
void saveClientData(int myno);
string globalinstr = "";
map<string, Client *> client_data;
map<string, string> groups;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        error(string("Args to start tracker- tracker_info and number"));
    }

    string my_ip_addr;
    string my_port_str;
    int my_port_no;
    int mytrackerno = atoi(argv[2]);
    {
        ifstream trackerInfoFile(argv[1]);
        for (int i = 0; i < mytrackerno; i++)
        {
            getline(trackerInfoFile, my_ip_addr);
            getline(trackerInfoFile, my_port_str);
        }
        my_port_no = atoi(my_port_str.c_str());
        trackerInfoFile.close();
    }
    // got the ipaddress and port number

    int my_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in my_addr;
    bzero((char *)&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(my_port_no);
    if (bind(my_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        error(string("Cannot bind my_sock_fd and my_addr"));
    }
    listen(my_sock_fd, 10);

    thread threadMan(Client_Thread_Manager, my_sock_fd, mytrackerno);

    string user_comm;
    while (true)
    {
        getline(cin, user_comm);
        if (user_comm == "quit")
        {
            globalinstr = "quit";
            break;
        }
    }
    close(my_sock_fd);
    return 0;
}

void Client_Thread_Manager(int my_sock_fd, int mytrackerno)
{
    struct sockaddr_in client_addr;
    socklen_t client_length = sizeof(client_addr);
    vector<int> client_sock_fds;
    vector<thread> client_threads;

    while (globalinstr != "quit")
    {
        bzero((char *)&client_addr, client_length);
        client_sock_fds.push_back(accept(my_sock_fd, (struct sockaddr *)&client_addr, &client_length));
        if (client_sock_fds.back() < 0)
        {
            error(string("Cannot accept socket connection" + to_string(client_sock_fds.size())));
        }
        thread thread_obj(ClientThread, client_sock_fds.back(), client_addr, mytrackerno);
        client_threads.push_back(move(thread_obj));
    }
}

void ClientThread(int client_sock_fd, struct sockaddr_in client_addr, int myno)
{
    bool loggedin = false;
    string user_id;
    char buffer[512 * 1024];

    ofstream trackerlog("trackerlog.txt");
    // whenever an instruction is read from client the inner loop breaks and restarts
    while (globalinstr != "quit")
    {
        bzero(buffer, sizeof(buffer));
        int n = 0;
        while (n <= 0 && globalinstr != "quit")
        {
            n = read(client_sock_fd, buffer, sizeof(buffer) - 1);
            stringstream ss(buffer);
            string command;
            ss >> command;

            trackerlog << buffer << endl;

            if (command == "create_user")
            {
                string passwd, ipaddr, portno;
                ss >> user_id >> passwd >> ipaddr >> portno;
                Client *newclient = new Client;
                newclient->user_id = user_id;
                newclient->passwd = passwd;
                newclient->ipaddress = ipaddr;
                newclient->portno = portno;
                trackerlog << newclient->user_id << " " << newclient->passwd << " " << newclient->ipaddress << " " << newclient->portno << endl;
                client_data[user_id] = newclient;
                string temp = "user created";
                write(client_sock_fd, temp.c_str(), temp.size());
            }
            else if (command == "login")
            {
                string passwd, ipaddr, portno;
                ss >> user_id >> passwd >> ipaddr >> portno;
                client_data[user_id]->ipaddress = ipaddr;
                client_data[user_id]->portno = portno;
                loggedin = true;
                string temp = "user logged in";
                write(client_sock_fd, temp.c_str(), temp.size());
            }
            else if (command == "create_group")
            {
                string group_id;
                ss >> group_id;
                client_data[user_id]->groups.push_back(group_id);
                groups[group_id] = user_id;
                string temp = "group created";
                write(client_sock_fd, temp.c_str(), temp.size());
            }
            else if (command == "join_group")
            {
                string group_id;
                ss >> group_id;
                string master = groups[group_id];
                string masterdata = client_data[master]->ipaddress + " " + client_data[master]->portno;
                write(client_sock_fd, masterdata.c_str(), masterdata.size());
            }
            else if (command == "logout")
            {
                return;
            }
            else if (command == "download_file")
            {
                string group_id;
                ss >> group_id;
                string master = groups[group_id];
                string masterdata = client_data[master]->ipaddress + " " + client_data[master]->portno;
                write(client_sock_fd, masterdata.c_str(), masterdata.size());
            }
        }
        saveClientData(myno);
    }

    trackerlog.close();
}

void saveClientData(int myno)
{
    ofstream f("clientData" + to_string(myno) + ".txt");
    for (auto it = client_data.begin(); it != client_data.end(); it++)
    {
        f << it->second->user_id << " " << it->second->passwd << " " << it->second->ipaddress << " " << it->second->portno << " ";
        for (int i = 0; i < it->second->groups.size(); i++)
        {
            f << it->second->groups[i] << " ";
        }
        f << endl;
    }
    f.close();

    f.open("groupmasters" + to_string(myno) + ".txt");
    for (auto it = groups.begin(); it != groups.end(); it++)
    {
        f << it->first << " " << it->second << endl;
    }
    f.close();
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