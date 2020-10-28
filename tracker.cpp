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
#include <sstream>
using namespace std;

void error(char *msg);
void error(string msg);
void Client_Thread_Manager(int my_sock_fd);
void ClientThread(int client_sock_fd, vector<vector<string>> &client_data);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        error("Need 3 arguments to start tracker");
    }
    // read tracker info
    ifstream trackerInfoFile(argv[1]);
    int mytrackerno = atoi(argv[2]);

    string my_ip_addr;
    string my_port_str;

    for (int i = 0; i < mytrackerno; i++)
    {
        getline(trackerInfoFile, my_ip_addr);
        getline(trackerInfoFile, my_port_str);
    }
    int my_port_no = atoi(my_port_str.c_str());
    trackerInfoFile.close();

    int my_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in my_addr;
    bzero((char *)&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(my_port_no);

    if (bind(my_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        error("Cannot bind my_sock_fd and my_addr");
    }

    listen(my_sock_fd, 10);

    thread threadMan(Client_Thread_Manager, my_sock_fd);

    string user_comm;
    while (true)
    {
        getline(cin, user_comm);
        if (user_comm == "quit")
        {
            break;
        }
    }
    close(my_sock_fd);
    return 0;
}

void Client_Thread_Manager(int my_sock_fd)
{
    vector<vector<string>> client_data;
    struct sockaddr_in client_addr;
    socklen_t client_length = sizeof(client_addr);
    vector<int> client_sock_fds;
    vector<thread> client_threads;
    while (true)
    {
        bzero((char *)&client_addr, client_length);
        client_sock_fds.push_back(accept(my_sock_fd, (struct sockaddr *)&client_addr, &client_length));
        if (client_sock_fds.back() < 0)
        {
            error(string("Cannot accept socket connection" + to_string(client_sock_fds.size())));
        }
        thread thread_obj(ClientThread, client_sock_fds.back(), client_data);
    }
}

void ClientThread(int client_sock_fd, vector<vector<string>> &client_data)
{
    thread::id this_id = this_thread::get_id();
    char buffer[512 * 1024];
    while (true)
    {
        bzero(buffer, sizeof(buffer));
        int n = 0;
        while (n <= 0)
        {
            if (write(client_sock_fd, buffer, sizeof(buffer) - 1) == -1)
            {
                close(client_sock_fd);
                return;
            }
            n = read(client_sock_fd, buffer, sizeof(buffer) - 1);

            stringstream ss(buffer);
            string command;
            ss >> command;
            if (command == "create_user")
            {
                string user_id, passwd;
                ss >> user_id >> passwd;
                ofstream f("trial.txt");
                f.write(user_id.c_str(), user_id.size());
                f.write(passwd.c_str(), passwd.size());
                f.close();
            }
            else if (command == "login")
            {
            }
            else if (command == "create_group")
            {
            }
            else if (command == "join_group")
            {
            }
            else if (command == "leave_group")
            {
            }
            else if (command == "logout")
            {
            }
            else if (command == "download_file")
            {
            }
            else if (command == "upload_file")
            {
            }
            else if (command == "list_groups")
            {
            }
        }
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