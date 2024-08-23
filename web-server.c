#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 80
#define WEBSITE_ROOT_PATH "/var/www/html"
#define MAX_LENGTH 8192
#define BUFFER_LENGTH 1024
#define DEFAULT_PAGE "/index.html"
#define PROTOCOL "HTTP/1.1"

void build_response(char *response, int status_code, char *response_body);
void *handle_request(void *);
void extract_path(char *http_request, char *path);

int main()
{

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    pthread_t request_thread;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 80
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 80
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 20) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            int *new_sock = malloc(sizeof(int)); // Allocate memory for the socket descriptor
            if (new_sock == NULL)
            {
                perror("Failed to allocate memory");
                exit(EXIT_FAILURE);
            }
            *new_sock = new_socket;
            pthread_create(&request_thread, NULL, handle_request, (void *)new_sock);
            // detach the thread
            pthread_detach(request_thread);
        }
    }
    // closing the listening socket
    close(server_fd);

    return 0;
}

void build_response(char *response, int status_code, char *response_body)
{
    sprintf(response, "%s %d OK\r\n\r\n%s\r\n", PROTOCOL, status_code, response_body);
}

void *handle_request(void *socket_desc)
{

    char buffer[BUFFER_LENGTH] = {0};
    char http_request[BUFFER_LENGTH] = {0};
    char response[MAX_LENGTH] = {0};
    char response_body[MAX_LENGTH] = {0};
    char path[BUFFER_LENGTH];
    char filepath[BUFFER_LENGTH];
    ssize_t valread;
    char *token;
    FILE *fptr;
    int new_socket = *((int*)socket_desc);  // Cast and dereference the pointer to get the socket descriptor
    free(socket_desc);  // Free the allocated memory after use

    valread = read(new_socket, http_request,
                   BUFFER_LENGTH - 1); // subtract 1 for the null
                                       // terminator at the end

    // Extracting the path from the HTTP request
    extract_path(http_request, path);

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
        while (fgets(buffer, BUFFER_LENGTH, fptr) != NULL)
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

    return 0;
}

void extract_path(char *http_request, char *path)
{
    char *token;
    token = strtok(http_request, " ");
    while (token != NULL)
    {
        if (strcmp(token, "GET") == 0 || strcmp(token, "POST") == 0)
        {
            token = strtok(NULL, " ");
            strcpy(path, token);
            break;
        }
        token = strtok(NULL, " ");
    }
}