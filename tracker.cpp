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
#include <atomic>
using namespace std;

void error(char *msg);
void error(string msg);
void Client_Thread_Manager(int my_sock_fd, int myno);
void ClientThread(int client_sock_fd, struct sockaddr_in client_addr, int myno);
void write_clientData_tofile(int myno);

vector<vector<string>> client_data;
// store as userid, passwd, ipaddr, portno, groups it created
string globalinstr = "";

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        error(string("Need 3 arguments to start tracker"));
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
    vector<vector<string>> client_data;
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
    for (int i = 0; i < client_threads.size(); i++)
    {
        client_threads[i].join();
    }
}

void ClientThread(int client_sock_fd, struct sockaddr_in client_addr, int myno)
{
    bool loggedin = false;
    string user_id;
    thread::id this_id = this_thread::get_id();
    char buffer[512 * 1024]; //where stuff from clients are stored

    ofstream logfile("logfile.txt");
    logfile.close();

    while (globalinstr != "quit")
    {
        bzero(buffer, sizeof(buffer));
        int n = 0;
        while (n <= 0 && globalinstr != "quit")
        {
            //if (write(client_sock_fd, buffer, sizeof(buffer) - 1) == -1)
            //{
            //    logfile.open("logfile.txt", ios_base::app);
            //    logfile << "no connection\n";
            //    logfile.close();
            //    close(client_sock_fd);
            //    return;
            //}

            n = read(client_sock_fd, buffer, sizeof(buffer) - 1);

            stringstream ss(buffer);
            string command;
            ss >> command;

            logfile.open("logfile.txt", ios_base::app);
            logfile << command << "\n";
            logfile.close();

            if (command == "create_user")
            {
                string passwd, ipaddr, portno;
                ss >> user_id >> passwd >> ipaddr >> portno;
                vector<string> t;
                t.push_back(user_id);
                t.push_back(passwd);
                //string ipaddr = to_string(client_addr.sin_addr.s_addr);
                //string portno = to_string(ntohs(client_addr.sin_port));
                t.push_back(ipaddr);
                t.push_back(portno);
                client_data.push_back(t);
                logfile.open("logfile.txt", ios_base::app);
                logfile << "user " << user_id << " " << passwd << " " << ipaddr << " " << portno << "\n";
                logfile.close();
                {
                    string temp = "created user";
                    write(client_sock_fd, temp.c_str(), temp.size());
                }
            }
            else if (command == "login")
            {
                string passwd, ipaddr, portno;
                ss >> user_id >> passwd >> ipaddr >> portno;
                for (int i = 0; i < client_data.size(); i++)
                {
                    if (client_data[i][0] == user_id)
                    {
                        //client_data[i][2] = to_string(client_addr.sin_addr.s_addr);
                        //client_data[i][3] = to_string(ntohs(client_addr.sin_port));
                        client_data[i][2] = ipaddr;
                        client_data[i][3] = portno;
                        i = client_data.size();
                    }
                }
                loggedin = true;
                //logfile.open("logfile.txt", ios_base::app);
                //logfile << "logged in\n";
                //logfile.close();
                {
                    string temp = "user logged in";
                    write(client_sock_fd, temp.c_str(), temp.size());
                }
            }
            else if (command == "create_group")
            {
                string group_id;
                ss >> group_id;
                for (int i = 0; i < client_data.size(); i++)
                {
                    if (user_id == client_data[i][0])
                    {
                        client_data[i].push_back(group_id);
                        i = client_data.size();
                    }
                }
                {
                    string temp = "group created";
                    write(client_sock_fd, temp.c_str(), temp.size());
                }
            }
            else if (command == "join_group")
            {
                string group_id;
                ss >> group_id;
                for (int i = 0; i < client_data.size(); i++)
                {
                    for (int j = 4; j < client_data[i].size(); j++)
                    {
                        if (client_data[i][j] == group_id)
                        {
                            string data = client_data[i][2] + " " + client_data[i][3];
                            int n = write(client_sock_fd, data.c_str(), data.size());
                            i = client_data.size();
                            j = client_data[i].size();
                        }
                    }
                }
            }
            else if (command == "leave_group")
            {
            }
            else if (command == "logout")
            {
                return;
            }
            else if (command == "download_file")
            {
                string group_id;
                ss >> group_id;
                bool found = false;
                for (int i = 0; i < client_data.size(); i++)
                {
                    for (int j = 4; j < client_data[i].size(); j++)
                    {
                        if (client_data[i][j] == group_id)
                        {
                            string data = client_data[i][2] + " " + client_data[i][3];
                            int n = write(client_sock_fd, data.c_str(), data.size());
                            found = true;
                            j = client_data[i].size();
                        }
                    }
                    if (found)
                    {
                        i = client_data.size();
                    }
                }
                //{
                //    string temp = "download accept";
                //    write(client_sock_fd, temp.c_str(), temp.size());
                //}
            }
            else if (command == "upload_file")
            {
            }
            else if (command == "list_groups")
            {
            }
            write_clientData_tofile(myno);
        }
    }
    close(client_sock_fd);
}

void write_clientData_tofile(int myno)
{
    ofstream f("clientData.txt");
    for (int i = 0; i < client_data.size(); i++)
    {
        for (int j = 0; j < client_data[i].size(); j++)
        {
            f << client_data[i][j] << " ";
        }
        f << "\n";
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