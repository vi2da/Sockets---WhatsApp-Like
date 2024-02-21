// vi2da

#include "whatsappio.h"
#include <map>
#include <netinet/in.h>
#include <netdb.h>
#include <zconf.h>
#include <iostream>


#define MAXHOSTNAME 256
#define NAME_TAKEN "name already in use"
#define ERROR_REPLY "error"
#define FILES_TO_MONITOR 4 // for select(). stdin, stdout, stderr, server
#define CLOSE_SOCKET "close"
#define SUCCESS_REPORT "success"

//=============================================================================
// helper functions:


void end_connection(int socket_fd)
{
    if(close(socket_fd) < 0)
    {
        print_error("close", errno);
    }
}

//raw_command, commandT, new_client_name, dest_name, message, clients, socket_fd

int handle_command(std::string& raw_command, command_type& commandT, std::string& client_name, std::string& dest_name,
                   std::string& message, std::vector<std::string>& clients, int server_sock_fd)
{
    std::string command_to_server = "";
    switch(commandT)
    {
        case EXIT:
        {
            command_to_server = "exit";
            if(write_data(server_sock_fd, command_to_server) < 0)
            {
                end_connection(server_sock_fd);
                exit(1);
            }

            std::string exit_status = read_data(server_sock_fd);
            if(exit_status == SUCCESS_REPORT)
            {
                print_exit(false, client_name);
                exit(0);
            }
        }
            break;

        case SEND:
        {
            if(dest_name == client_name)
            {
                print_send(false, false, client_name, dest_name, message);
            }
            else
            {
                command_to_server = raw_command;
                if(write_data(server_sock_fd, command_to_server) < 0)
                {
                    end_connection(server_sock_fd);
                    exit(1);
                }

                std::string send_status = read_data(server_sock_fd);
                if(send_status == ERROR_REPLY)
                {
                    print_send(false, false, client_name, dest_name, message);
                }
                else
                {
                    print_send(false, true, client_name, dest_name, message);
                }
            }
        }
            break;

        case CREATE_GROUP:
        {
            // the sender cant be also the reciever
            // check that we send to sever list of least 2 member.
            int num_member = clients.size();
            for(auto name : clients)
            {
                if(name == client_name)
                {
                    num_member--;
                }
            }

            if((num_member < 1) || (client_name == dest_name))
            {
                print_create_group(false, false, client_name, dest_name);
            }
            else
            {
                command_to_server = raw_command;
                if(write_data(server_sock_fd, command_to_server) < 0)
                {
                    end_connection(server_sock_fd);
                    exit(1);
                }

                std::string create_group_status = read_data(server_sock_fd);
                if(create_group_status == ERROR_REPLY)
                {
                    print_create_group(false, false, client_name, dest_name);
                }
                else
                {
                    print_create_group(false, true, client_name, dest_name);
                }
            }
        }
            break;

        case WHO:
        {
            command_to_server = "who";
            if(write_data(server_sock_fd, command_to_server) < 0)
            {
                end_connection(server_sock_fd);
                exit(1);
            }

            const std::string clients_list = read_data(server_sock_fd);
            if(clients_list == ERROR_REPLY)
            {
                print_who_client(false, clients_list);
            }
            else
            {
                print_who_client(true, clients_list);
            }
        }
            break;

        default:
            print_invalid_input();
    }
}

//=============================================================================
// the main function that runs the program

int main(int argc, char *argv[])
{
    // expects 3 args (+program name)
    // for example: whatsappClient Naama 132.65.125.59 8875
    if(argc != 4)
    {
        print_client_usage();
        //todo: should we exit in this case?
        exit(1);
    }

    // if we arrived at this point, all argumets passed correctly
    std::string new_client_name = argv[1];
    std::string IP_address = argv[2];
    int port_num = atoi(argv[3]);

    // parse name and check if in legal format:
    if((new_client_name.size() > WA_MAX_NAME) || !is_legal_name(&new_client_name))
    {
        print_client_usage();
        exit(1);
    }

    // open a socket so send the new client name to server, in order for it to check
    // if the name is already in use
    char hostname[MAXHOSTNAME+1];
    struct sockaddr_in sa = {0};
    struct hostent* hp = nullptr;
    int socket_fd;

    // todo delete: hostname: nak-05

    gethostname(hostname, MAXHOSTNAME);
    hp = gethostbyname(hostname);
    if (hp == nullptr)
    {
        print_error("gethostname", h_errno);
        exit(1);
    }


    // sockaddrr_in initlization
    memset(&sa, 0, sizeof(sa));

    // this is our host address
    memcpy((char *)&sa.sin_addr, hp->h_addr,(size_t)hp->h_length);

    sa.sin_family= (sa_family_t)hp->h_addrtype;

    // this is our port number
    sa.sin_port= htons((u_short)port_num);

    // create an endpoint for communication (socket for the client)
    if((socket_fd = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
    {
        print_error("socket", errno);
        exit(1);
    }

    // connects the socket referred to by the file descriptor to the address specified by &sa
    // i.e, connecting the client socket to the server's listening socket
    if(connect(socket_fd, (struct  sockaddr*)&sa, sizeof(sa)) < 0)
    {
        close(socket_fd);
        print_error("connect", errno);
        exit(1);
    }

    // if we arrived here, we managed to connect to the server's listening socket,
    // and now we can send the server the client name to check if it is already taken:

    if(write_data(socket_fd, new_client_name) < 0)
    {
        end_connection(socket_fd);
        exit(1);
    }

    std::string server_reply = read_data(socket_fd);
    if(server_reply == ERROR_REPLY)
    {
        //todo print error and exit
    }

    if(server_reply == NAME_TAKEN)
    {
        print_dup_connection();
        exit(1);
    }

    // if we arrived at this point, the server replied that the name is available
    // and we are now connected to the server
    print_connection();

    // select flow. taken from TA 9, slide 60
    fd_set clients_fds;
    fd_set read_fds;

    FD_ZERO(&clients_fds);
    FD_SET(socket_fd, &clients_fds);
    FD_SET(STDIN_FILENO, &clients_fds);

    // params for parse_command() from whatsappio.h
    command_type commandT = INVALID;
    std::string dest_name;
    std::string message;
    std::vector<std::string> clients;

    while(true)
    {
        read_fds = clients_fds;
        // blocks the calling process until there is activity on any of the
        // fds in client_fds.
        // if select() fails we end the connection
        int ready = select(FILES_TO_MONITOR, &read_fds, NULL, NULL, NULL);
        if(ready < 0)
        {
            print_error("select", errno);
            end_connection(socket_fd);
            exit(1);
        }

        // waiting for the client to enter a command
        if(FD_ISSET(STDIN_FILENO, &read_fds))
        {
            std::string raw_command;
            getline(std::cin, raw_command);
            parse_command(raw_command, commandT, dest_name, message, clients);
            handle_command(raw_command, commandT, new_client_name, dest_name, message, clients, socket_fd);
        }
        else
        {
            std::string server_msg = read_data(socket_fd);
            if(server_msg.size() == 0 || server_reply == CLOSE_SOCKET)
            {
                if(close(socket_fd) < 0)
                {
                    print_error("close", errno);
                    exit(1);
                }
            }

            else
            {
                std::cout << server_msg << std::endl;
            }
        }
    }
}