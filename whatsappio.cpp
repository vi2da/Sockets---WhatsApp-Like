// vi2da

#include "whatsappio.h"
#include <unistd.h>

void print_exit() {
    printf("EXIT command is typed: server is shutting down\n");
}

void print_connection() {
    printf("Connected Successfully.\n");
}

void print_connection_server(const std::string& client) {
    printf("%s connected.\n", client.c_str());
}


void print_dup_connection() {
    printf("Client name is already in use.\n");
}

void print_fail_connection() {
    printf("Failed to connect the server\n");
}

void print_server_usage() {
    printf("Usage: whatsappServer portNum\n");
}

void print_client_usage() {
    printf("Usage: whatsappClient clientName serverAddress serverPort\n");
}

void print_create_group(bool server, bool success, 
                        const std::string& client, const std::string& group) {
    if(server) {
        if(success) {
            printf("%s: Group \"%s\" was created successfully.\n", 
                   client.c_str(), group.c_str());
        } else {
            printf("%s: ERROR: failed to create group \"%s\"\n", 
                   client.c_str(), group.c_str());
        }
    }
    else {
        if(success) {
            printf("Group \"%s\" was created successfully.\n", group.c_str());
        } else {
            printf("ERROR: failed to create group \"%s\".\n", group.c_str());
        }
    }
}

void print_send(bool server, bool success, const std::string& client, 
                const std::string& name, const std::string& message) {
    if(server) {
        if(success) {
            printf("%s: \"%s\" was sent successfully to %s.\n", 
                   client.c_str(), message.c_str(), name.c_str());
        } else {
            printf("%s: ERROR: failed to send \"%s\" to %s.\n", 
                   client.c_str(), message.c_str(), name.c_str());
        }
    }
    else {
        if(success) {
            printf("Sent successfully.\n");
        } else {
            printf("ERROR: failed to send.\n");
        }
    }
}

void print_message(const std::string& client, const std::string& message) {
    printf("%s: %s\n", client.c_str(), message.c_str());
}

void print_who_server(const std::string& client) {
    printf("%s: Requests the currently connected client names.\n", client.c_str());
}

void print_who_client(bool success, const std::string& clients) {
    if(success) {
        printf(clients.c_str());
        printf("\n");
    } else {
        printf("ERROR: failed to receive list of connected clients.\n");
    }
}

void print_exit(bool server, const std::string& client) {
    if(server) {
        printf("%s: Unregistered successfully.\n", client.c_str());
    } else {
        printf("Unregistered successfully.\n");
    }
}

void print_invalid_input() {
    printf("ERROR: Invalid input.\n");
}

void print_error(const std::string& function_name, int error_number) {
    printf("ERROR: %s %d.\n", function_name.c_str(), error_number);
}

void parse_command(const std::string& command, command_type& commandT, 
                   std::string& name, std::string& message, 
                   std::vector<std::string>& clients) {
    if(command.empty())
    {
        commandT = INVALID;
        return;
    }
    char c[WA_MAX_INPUT];
    const char *s; 
    char *saveptr;
    name.clear();
    message.clear();
    clients.clear();
    
    strcpy(c, command.c_str());
    s = strtok_r(c, " ", &saveptr);
    
    if(!strcmp(s, "create_group")) {
        commandT = CREATE_GROUP;
        s = strtok_r(NULL, " ", &saveptr);
        if(!s) {
            commandT = INVALID;
            return;
        } else {
            name = s;
            while((s = strtok_r(NULL, ",", &saveptr)) != NULL) {
                clients.emplace_back(s);
            }
        }
    } else if(!strcmp(s, "send")) {
        commandT = SEND;
        s = strtok_r(NULL, " ", &saveptr);
        if(!s) {
            commandT = INVALID;
            return;
        } else {
            name = s;
            if(command.size() <name.size() + 6)
            {
                commandT = INVALID;
                return;
            }
            message = command.substr(name.size() + 6); // 6 = 2 spaces + "send"
        }
    } else if(!strcmp(s, "who")) {
        commandT = WHO;
    } else if(!strcmp(s, "exit")) {
        commandT = EXIT;
    } else {
        commandT = INVALID;
    }
}


/**
 * Read data from a given socket FD.
 * @param s the socket FD.
 * @return client message on success, empty string on failure.
 */
std::string read_data(int s)
{
    char message[WA_MAX_MESSAGE +1];
    char* buf = message;
    int bcount = 0;
    int br = 0;

    while ( (bcount < (WA_MAX_MESSAGE + 1)) )
    {
        br = (int) read(s, buf, (size_t)(WA_MAX_MESSAGE + 1) - bcount);
        if (br > 0)
        {
            bcount += br;
            buf += br;
        }

        if (br < 1)
        {
            print_error("read", errno);
            return "";
        }
    }

    std::string result(message);
    return result;
}


/**
 * Write data to a given socket FD.
 * @param s the socket FD.
 * @param message the message to send.
 * @return number bytes written on success, -1 on failure.
 */
int write_data(int s, std::string message)
{
    message.resize(WA_MAX_MESSAGE + 1, '\0');
    const char* buf = message.c_str();
    int bcount = 0;
    int br = 0;

    while (bcount < (WA_MAX_MESSAGE + 1))
    {
        br = (int) write(s, buf, (size_t)(WA_MAX_MESSAGE + 1) - bcount);
        if (br > 0)
        {
            bcount += br;
            buf += br;
        }
        if (br < 1)
        {
            print_error("write", errno);
            return -1;
        }
    }

    return(bcount);
}

int is_legal_name(std::string* name)
{

    const char* char_arr = (*name).c_str();
    for(int i = 0; i < name->size(); i++)
    {
        // the name contains a character which is not alphanumeric
        if(!isalnum(char_arr[i]))
        {
            return 0;
        }
    }

    return 1;
}