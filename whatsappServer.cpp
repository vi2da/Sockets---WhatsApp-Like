// vi2da

#include <iostream>
#include <sys/select.h>
#include <climits>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <list>
#include <algorithm>
#include "whatsappio.h"


#define NAME_ALREADY_IN_USE "name already in use"
#define SUCCESS_REPORT "success"
#define ERROR_REPORT "error"



int server_socket;
fd_set fds;
std::map<std::string, int> fd_of_clients;
std::map<std::string, std::list<int>> friend_of_group;

int run_config_server(int port_num);
void endale_request_client(fd_set fds);
void parse_client_request(int fd, std::string client_name);
void terminate_server();
int start_to_listen();
void new_member_conect();

//_____________________________________________________________________________//

int main(int argc, char *argv[]) {
    // Check arguments for running whatsappServer program.
    if (argc != 2)
    {
        print_server_usage();
        return 0;
    }

    // Create a welcome socket for clients to connect to.
    if ( (server_socket = run_config_server(std::atoi(argv[1]))) == -1)
    {
        std::exit(1);
    }

    return start_to_listen();
    // Collect FDs for select() to listen to.

}

int run_config_server(int port_num)
{
    int my_socket;
    char my_host_name[HOST_NAME_MAX + 1];
    struct hostent *hp;
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sockaddr_in));

    if(gethostname(my_host_name, HOST_NAME_MAX) == -1)
    {
        print_error("gethostname", errno);
        return -1;
    }

    hp = gethostbyname(my_host_name);
    if(hp == NULL)
    {
        print_error("gethostbyname", errno);
        return -1;
    }


    sa.sin_family = (sa_family_t) hp->h_addrtype;
    sa.sin_port = htons(port_num);
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket == -1)
    {
        print_error("socket", errno);
        return -1;
    }

    if(bind(my_socket, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1)
    {
        print_error("bind", errno);
        return -1;
    }

    if(listen(my_socket, 5) == -1)
    {
        print_error("listen", errno);
        return -1;
    }

    return my_socket;
}

int start_to_listen()
{
    FD_ZERO(&fds);
    FD_SET(server_socket, &fds);
    FD_SET(STDIN_FILENO, &fds);

    fd_set active_read_sockets;
    while (true)
    {
        active_read_sockets = fds;
        int max_fd = 3;
        for(auto it_fd = fd_of_clients.begin(); it_fd != fd_of_clients.end(); it_fd++)
        {
            if(it_fd->second > max_fd)
            {
                max_fd = it_fd->second;
            }
        }
        if (select((int) (max_fd + 1), &active_read_sockets, NULL, NULL, NULL) < 0)
        {
            print_error("select", errno);
            terminate_server();
            return -1;
        }

        endale_request_client(active_read_sockets);
    }
}


void terminate_server()
{
    // Close all client sockets.
    for (auto it = fd_of_clients.begin(); it != fd_of_clients.end(); it++)
    {
        write_data(it->second, "close");     // Tell client to terminate.
        if (close((it->second)) == -1)                  // Close client socket.
        {
            print_error("close", errno);
        }
    }

    // Close server welcome socket.
    if (close(server_socket) == -1)
    {
        print_error("close", errno);
    }

    std::exit(0);
}


void endale_request_client(fd_set fds)
{

    // If there is a pending client that wants to connect.
    if (FD_ISSET(server_socket, &fds))
    {
        new_member_conect();
    }

        // If a command was typed in std input.
    else if (FD_ISSET(STDIN_FILENO, &fds))
    {
        std::string msg;
        getline(std::cin, msg);
        if (msg == "EXIT")
        {
            print_exit();
            terminate_server();
        }
        else
        {
            print_invalid_input();
        }
    }

        // Else - some client sent us message.
    else
    {
        // Find the client that sent the message.
        for (auto it = fd_of_clients.begin(); it != fd_of_clients.end(); it++)
        {
            if (FD_ISSET(it->second, &fds))
            {
                // Hadle client request.
                parse_client_request(it->second, it->first);

                // Don't handle rest of clients - go back to select(), because clients
                // list and clients_fds might have been altered.
                break;
            }
        }
    }
}


void new_member_conect()
{
    int newClientFD;
    if ((newClientFD = accept(server_socket,NULL,NULL)) > -1)
    {
        std::string newClientName = read_data(newClientFD);
        if (newClientName.length() > 0)
        {
            if (  (fd_of_clients.find(newClientName) != fd_of_clients.end())
                 || (friend_of_group.find(newClientName) != friend_of_group.end()) )
            {
                write_data(newClientFD, NAME_ALREADY_IN_USE);
            }
            else
            {
                fd_of_clients[newClientName] = newClientFD;
                FD_SET(newClientFD, &fds);
                write_data(newClientFD, SUCCESS_REPORT);
                print_connection_server(newClientName.c_str());
                return;
            }
        }
        else
        {
            write_data(newClientFD, ERROR_REPORT);
        }

        if (close(newClientFD) == -1)
        {
            print_error("close", errno);
        }
    }
    else
    {
        print_error("accept", errno);
    }
}

void parse_client_request(int fd, std::string client_name)
{

    const std::string& command = read_data(fd);
    command_type commandT;
    std::string name;
    std::string message;
    std::vector<std::string> clients;

    parse_command(command, commandT, name, message, clients);

    if(commandT == CREATE_GROUP)
    {
        if ( (fd_of_clients.find(name) == fd_of_clients.end())
             && (friend_of_group.find(name) == friend_of_group.end()) )
        {
            int count_member = 0;
            std::list<int> clients_list;

            sort( clients.begin(), clients.end() );
            clients.erase(unique( clients.begin(), clients.end() ), clients.end() );

            for (auto member : clients)
            {
                if (fd_of_clients.find(member) != fd_of_clients.end())
                {
                    if (member != client_name)
                    {
                        clients_list.push_back(fd_of_clients[member]);
                        count_member++;
                    }
                }
                else
                {
                    print_create_group(true, false, client_name, name);
                    write_data(fd, ERROR_REPORT);
                    return;
                }
            }

            if(count_member < 1)
            {
                print_create_group(true, false, client_name, name);
                write_data(fd, ERROR_REPORT);
                return;
            }

            clients_list.push_back(fd);

            friend_of_group[name] = clients_list;
            print_create_group(true, true, client_name, name);
            write_data(fd, SUCCESS_REPORT);
        }
        else
        {
            print_create_group(true, false, client_name, name);
            write_data(fd, ERROR_REPORT);
        }
    }
    else if(commandT == SEND)
    {
        auto it_client = fd_of_clients.find(name);
        auto it_group = friend_of_group.find(name);
        if(it_client != fd_of_clients.end())
        {
            if(it_client->second == fd)
            {
                write_data(fd, ERROR_REPORT);
                print_send(true, false, client_name, it_client->first, message);
            }
            else
            {
                write_data(it_client->second, client_name + ": " + message);
                write_data(fd, SUCCESS_REPORT);
                print_send(true, true, client_name, it_client->first, message);

            }
        }
        else if(it_group != friend_of_group.end())
        {
            bool not_in_group = true;
            for(auto it_fd = it_group->second.begin(); it_fd != it_group->second.end(); it_fd++)
            {
                if((*it_fd) == fd)
                {
                    not_in_group = false;
                    break;
                }
            }

            if(not_in_group)
            {
                write_data(fd, ERROR_REPORT);
                print_send(true, false, client_name, name, message);
                return;
            }

            for(auto it_fd = it_group->second.begin(); it_fd != it_group->second.end(); it_fd++)
            {
                if((*it_fd) == fd)
                {
                    continue;
                }
                write_data( (*it_fd), client_name + ": " + message);
            }
            write_data(fd, SUCCESS_REPORT);
            print_send(true, true, client_name, name, message);
        }
        else
        {
            write_data(fd, ERROR_REPORT);
            print_send(true, false, client_name, name, message);
        }
    }
    else if(commandT == WHO)
    {
        print_who_server(client_name);
        std::string clients_to_write = "";
        for(auto it = fd_of_clients.begin(); it != fd_of_clients.end(); it++)
        {
            clients_to_write += ("," + it->first);
        }
        clients_to_write += ".";
        clients_to_write.erase(0, 1);
        write_data(fd, clients_to_write);
    }
    else if(commandT == EXIT)
    {
        write_data(fd, SUCCESS_REPORT);
        std::vector<std::string> empty_group_names;
        for(auto it = friend_of_group.begin(); it != friend_of_group.end(); it++)
        {
            (it->second).remove(fd);
            if((it->second).empty())
            {
                empty_group_names.push_back(it->first);
            }
        }

        for(auto group_name : empty_group_names)
        {
            friend_of_group.erase(group_name);
        }


        fd_of_clients.erase(client_name);

        print_exit(true, client_name);
        FD_CLR(fd, &fds);
        close(fd);
    }
    else if(commandT == INVALID)
    {
        //write_data(fd, ERROR_REPORT);
    }
}