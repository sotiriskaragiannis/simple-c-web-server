#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 80
#define MAX_LENGTH 1024
#define WEBSITE_ROOT_PATH "/var/www/html"
#define DEFAULT_PAGE "/index.html"
#define PROTOCOL "HTTP/1.1"

void build_response(char *response, int status_code, char *response_body);
void handle_request(int new_socket);

int main() {

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 80
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 80
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        handle_request(new_socket);
    }
    // closing the listening socket
    close(server_fd);

    return 0;
}

void build_response(char *response, int status_code, char *response_body) {
    sprintf(response, "%s %d OK\r\n\r\n%s\r\n", PROTOCOL, status_code, response_body);
}

void handle_request(int new_socket) {

    char buffer[MAX_LENGTH] = {0};
    char response[MAX_LENGTH] = {0};
    char response_body[MAX_LENGTH] = {0};
    ssize_t http_request;
    char *token;
    char *path;
    FILE *fptr;
    char filepath[MAX_LENGTH];

    http_request = read(new_socket, buffer,
                        1024 - 1); // subtract 1 for the null
                                   // terminator at the end

    // Extracting the path from the HTTP request
    token = strtok(buffer, " ");
    while (token != NULL)
    {
        if (strcmp(token, "GET") == 0 || strcmp(token, "POST") == 0)
        {
            token = strtok(NULL, " ");
            path = token;
            break;
        }
        token = strtok(NULL, " ");
    }

    // If the path is directory, then we will serve the default page
    if (strcmp(path, "/") == 0)
    {
        sprintf(filepath, "%s%s", WEBSITE_ROOT_PATH, DEFAULT_PAGE);
    }
    else
    {
        sprintf(filepath, "%s%s", WEBSITE_ROOT_PATH, path);
    }

    fptr = fopen(filepath, "r");
    if (fptr == NULL)
    {
        printf("Requested path: %s not found\n", path);
        build_response(response, 404, "Requested path not found");
    }
    else
    {
        // reading the file content
        while (fgets(buffer, MAX_LENGTH, fptr) != NULL)
        {
            sprintf(response_body, "%s%s", response_body, buffer);
        }
        build_response(response, 200, response_body);
        // clearing the response and response_body and closing the file
        fclose(fptr);
        // closing the connected socket
    }
    send(new_socket, response, strlen(response), 0);
    close(new_socket);
}