#ifndef FUNCTION_UTIL
#define FUNCTION_UTIL

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>
#include <sys/select.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cctype>


#include "stringConstant.h"
#include "defineStruct.h"
#include "functionUtil.h"

using std::string;
using std::vector;
using std::max;
using std::stringstream;


void generate_ip_port_pair_c_str(const client_info &client, string &ip_port_pair_string)
{
    char buff[20];
    inet_ntop(AF_INET, &client.ip_port.sin_addr.s_addr, buff, sizeof(buff));
    ip_port_pair_string += buff;
    ip_port_pair_string += "/";

    sprintf(buff, "%hu", htons(client.ip_port.sin_port));
    ip_port_pair_string += buff;
    std::cout << "ip_port_pair_string: " << ip_port_pair_string << "\n";
    std::cout << "ip_port_pair_string_c_tr: " << ip_port_pair_string.c_str() << "\n";
}

string find_user_name_by_sockfd(vector<client_info> &clients, int client_socket_fd)
{
    for(auto client : clients)
    {
        if(client.socket_fd == client_socket_fd)
            return client.name;
    }
}

void clear_offline_user(vector<client_info> &clients, int client_socket_fd)
{
    string offline_msg = find_user_name_by_sockfd(clients, client_socket_fd) + " is offline.\n";
    auto old_clients = clients;
    clients.clear();
    for(auto client : old_clients)
    {
        if(client.socket_fd == client_socket_fd)
        {
            close(client_socket_fd);
        }
        else
        {
            clients.push_back(client);
            write(client.socket_fd, offline_msg.c_str(), offline_msg.size());
        }
    }
}

int find_user_name_by_string_name(vector<client_info> &clients, const string &name)
{
    for(auto client : clients)
    {
        if(client.name == name)
            return client.socket_fd;
    }
    return -1;
}

int max_fd(vector<client_info> &clients)
{
    int temp_fd = 0;
    printf("temp_fd: %d\n", temp_fd);
    for(auto client : clients)
    {
        if(client.socket_fd > temp_fd)
            temp_fd = client.socket_fd;
    }
    return temp_fd;
}

int send_welcome_and_add_client(int server_socket_fd, vector<client_info> &clients)
{
    client_info new_client;
    //bzero(&new_client.ip_port, sizeof(sockaddr_in));
    memset(&new_client.ip_port, 0, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    new_client.socket_fd =
        accept(server_socket_fd, (sockaddr*)&new_client.ip_port, &len);
    
    string ip_port_pair_string;
    generate_ip_port_pair_c_str(new_client, ip_port_pair_string);

    string new_come_message = NEW_COME_STR + ip_port_pair_string + "\n";

    clients.push_back(new_client);
    write(new_client.socket_fd, new_come_message.c_str(), strlen(new_come_message.c_str()));

    return new_client.socket_fd;
}

void brocast_to_others(int new_client_sockfd, vector<client_info> &clients)
{
    for(int i = 0; i < clients.size(); i++)
    {
        if(clients[i].socket_fd == new_client_sockfd)
            continue;
        else
        {
            write(clients[i].socket_fd, BROCAST_STR, strlen(BROCAST_STR));
        }
    }
}

void parse_command_and_send_message(vector<client_info> &clients, char *command_from_client, int client_socket_fd)
{
    stringstream SS;
    SS << command_from_client;
    std::cout << "command's content: " << SS.str() << "\n";
    if(SS.str() == "\n") // THE user just type a fucking enter
        return;
    if(SS.str() == "\r\n") //Fuck you windows
        return;

    string command;
    SS >> command;
    if(command == "who")
    {
        //write(client_socket_fd, "WHO!!!\n", strlen("WHO!!!\n"));
        string list;
        for(auto client : clients)
        {
            list += SERVER_PROMPT;
            list += " ";
            list += client.name;
            list += " ";
            string ip_port_pair_string;
            //list += generate_ip_port_pair_c_str(client);
            generate_ip_port_pair_c_str(client, ip_port_pair_string);
            list += ip_port_pair_string;
            if(client.socket_fd == client_socket_fd)
                list += " -> me";
            list += "\n";
        }
        write(client_socket_fd, list.c_str(), list.size());
    }
    else if(command == "yell")
    {
        string yell_arg;
        string temp_arg;
        while(SS >> temp_arg)
        {
            //SS >> temp_arg;
            yell_arg += temp_arg + " ";
        }

        //write(client_socket_fd, "YELL!!\n", strlen("YELL!!\n"));
        string yell_user = find_user_name_by_sockfd(clients, client_socket_fd);
        for(auto client: clients)
        {
            string yell_msg = SERVER_PROMPT;
            yell_msg += " ";
            yell_msg += yell_user;
            yell_msg += " ";
            yell_msg += "yell ";
            //yell_msg += string(command_from_client).substr(5);
            yell_msg += yell_arg + "\n";
            write(client.socket_fd, yell_msg.c_str(), yell_msg.size());
        }
    }
    else if(command == "tell")
    {
        string user_want_to_tell;
        SS >> user_want_to_tell;
        int user_want_to_tell_sockfd;
        //write(client_socket_fd, "TELL!!\n", strlen("TELL!!\n"));
        if(find_user_name_by_sockfd(clients, client_socket_fd) == "anonymous")
        {
            write(client_socket_fd, YOU_ANONYMOUS, strlen(YOU_ANONYMOUS));
        }
        else if(user_want_to_tell == "anonymous")
        {
            write(client_socket_fd, USER_ANONYMOUS, strlen(USER_ANONYMOUS));
        }
        else if((user_want_to_tell_sockfd = 
                    find_user_name_by_string_name(clients, user_want_to_tell)) == -1)
        {
            write(client_socket_fd, USER_NOT_EXIST, strlen(USER_NOT_EXIST));
        }
        else
        {
            write(client_socket_fd, SUCCESS_MSG, strlen(SUCCESS_MSG));
            string tell_arg;
            string temp_arg;
            while(SS >> temp_arg)
            {
                //SS >> temp_arg;
                tell_arg += temp_arg + " ";
            }
            string tell_msg;
            tell_msg += "[Server] ";
            tell_msg += find_user_name_by_sockfd(clients, client_socket_fd) + " ";
            tell_msg += "tell you ";
            tell_msg += tell_arg + "\n";
            write(user_want_to_tell_sockfd, tell_msg.c_str(), tell_msg.size());
        }
    }
    else if(command == "name")
    {
        //write(client_socket_fd, "NAME!!\n", strlen("NAME!!\n"));
        string name_want_to_change;
        SS >> name_want_to_change;
        int i = 0;
        if(name_want_to_change == "anonymous")
        {
            write(client_socket_fd, NEW_NAME_ANONYMOUS, strlen(NEW_NAME_ANONYMOUS));
            return;
        }
        for(; i < name_want_to_change.size(); i++)
        {
            if(!isalpha(name_want_to_change[i]))
            {
                i = 10000;
                break;
            }
        }
        if(i < 2 || i >= 12)
        {
            write(client_socket_fd, NAME_NOT_2to12_ENG, strlen(NAME_NOT_2to12_ENG));
        }
        else if(find_user_name_by_string_name(clients, name_want_to_change) != -1)
        {
            string FUCKING_USED;
            FUCKING_USED += "[Server] ERROR: ";
            FUCKING_USED += name_want_to_change + " ";
            FUCKING_USED += "has been used by others.\n";
            write(client_socket_fd, FUCKING_USED.c_str(), FUCKING_USED.size());
        }
        else
        {
            string TELL_OTHER_CHANGE_NAME = "[Server] ";
            string old_name = find_user_name_by_sockfd(clients, client_socket_fd);
            TELL_OTHER_CHANGE_NAME += old_name + " ";
            TELL_OTHER_CHANGE_NAME += "is now known as ";
            TELL_OTHER_CHANGE_NAME += name_want_to_change + "\n";
            for(int i = 0; i < clients.size(); i++)
            {
                if(client_socket_fd == clients[i].socket_fd)
                {
                    clients[i].name = name_want_to_change;
                }
                else
                {
                    //string TELL_OTHER_CHANGE_NAME = "[Server] <OLD USERNAME> is now known as <NEW USERNAME>.\n";
                    write(clients[i].socket_fd, TELL_OTHER_CHANGE_NAME.c_str(), TELL_OTHER_CHANGE_NAME.size());        
                }
            }
            string CHANGE_NAME_MSG = "[Server] You're now known as ";
            CHANGE_NAME_MSG += name_want_to_change + "\n";
            write(client_socket_fd, CHANGE_NAME_MSG.c_str(), CHANGE_NAME_MSG.size());
        }
        
    }
    else
    {
        write(client_socket_fd, ERR_COMMAND, strlen(ERR_COMMAND));
    }
}

#endif