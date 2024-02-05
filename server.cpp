#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "127.0.0.1"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    uint16_t server_port;

    char *backend_host;
    uint16_t backend_port;
};


void parse_args(int argc, char *argv[], struct server_app *app);
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const std::string& path);
void proxy_remote_file(struct server_app *app, int client_socket, const std::string& request);

std::string receive_client_request(int socket);
std::string parse_filename_from_request(const std::string& request);
std::string decode_uri(const std::string &value);
std::string get_file_extension(const std::string& str);


int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->backend_host = NULL;
    app->backend_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->backend_host = strdup(optarg);
            break;
        case 'p':
            app->backend_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->backend_host == NULL) {
        app->backend_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

std::vector<std::string> split(std::string str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    while ((pos = str.find(delim)) != std::string::npos) {
        std::string token = str.substr(0, pos);
        tokens.push_back(token);
        str.erase(0, pos + delim.size());
    }
    return tokens;
}

void handle_request(struct server_app *app, int client_socket) {
    std::string request = receive_client_request(client_socket);
    std::string filename = parse_filename_from_request(request);

    if (get_file_extension(filename) == "ts") {
        proxy_remote_file(app, client_socket, request);
    }
    else {
        serve_local_file(client_socket, filename);
    }
}

std::string receive_client_request(int socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {// Connection closed or error
        exit(1);
    }

    buffer[bytes_read] = '\0';
    return std::string(buffer);
}

std::string parse_filename_from_request(const std::string& request) {

    std::vector<std::string> requestTokens = split(request, "\r\n");
    std::vector<std::string> requestLineTokens = split(requestTokens[0], " ");

    for (auto token : requestTokens) {
        std::cout << token << "\n";
    }

    std::string requestURI = requestLineTokens[1];
    requestURI.erase(0,1);

    requestURI = decode_uri(requestURI);
    return requestURI == "" ? "index.html" : requestURI;

}

std::string decode_uri(const std::string &uri) {
    std::string decoded;
    std::stringstream ss(uri);

    while (!ss.eof()) {
        int hex;
        char c;
        
        while (ss.peek() != '%' && !ss.eof()) {
            decoded += ss.get();
        }

        if (ss.peek() == '%') {
            
            ss.ignore(); // Skip '%'
            ss >> std::hex >> hex; // Read two hexadecimal characters
            c = static_cast<char>(hex); // Convert hexadecimal value to character
            decoded += c; 
        }
    }

    return decoded;
}

std::string get_file_extension(const std::string& str) {
    for (auto it = str.end(); it >= str.begin(); it--) {
        if (*it == '.') {
            return std::string(it+1, str.end());
            break;
        }
    }
    return "";
}

void serve_local_file(int client_socket, const std::string& path) {

    std::string extension = get_file_extension(path);

    std::string response;
    std::fstream file(path);
    if (!file.good()) {
        response = "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: 15\r\n"
            "\r\n"
            "File not found.";
    }

    else if (extension == "html") {
        response = "HTTP/1.0 200 OK\r\n"
                      "Content-Type: text/html; charset=UTF-8\r\n"
                      "Content-Length: ";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        response.append(std::to_string(fileContent.length()) + "\r\n\r\n");
        response.append(fileContent);
    }

    else if (extension == "txt") {
        response = "HTTP/1.0 200 OK\r\n"
                      "Content-Type: text/plain; charset=UTF-8\r\n"
                      "Content-Length: ";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        response.append(std::to_string(fileContent.length()) + "\r\n\r\n");
        response.append(fileContent);
    }
    else if (extension == "jpg") {
        response = "HTTP/1.0 200 OK\r\n"
                      "Content-Type: image/jpeg\r\n"
                      "Content-Length: ";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        response.append(std::to_string(fileContent.length()) + "\r\n\r\n");
        response.append(fileContent);
    }
    else {
        response = "HTTP/1.0 200 OK\r\n"
                      "Content-Type: application/octet-stream\r\n"
                      "Content-Length: ";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        response.append(std::to_string(fileContent.length()) + "\r\n\r\n");
        response.append(fileContent);
    }

    send(client_socket, response.c_str(), response.length(), 0);
}

void proxy_remote_file(struct server_app *app, int client_socket, const std::string& request) {

    int backend_socket;
    if ((backend_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "socket creation error\n";
        char response[] = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(app->backend_port);

    if (inet_pton(AF_INET, app->backend_host, &backend_addr.sin_addr) <= 0) {
        std::cout << "invalid address/address not supported\n";
        char response[] = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    int status;
    if ((status = connect(backend_socket, (struct sockaddr*)&backend_addr, sizeof(backend_addr)))) {
        std::cout << "backend connection failed\n";
        char response[] = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    send(backend_socket, request.c_str(), request.length(), 0);
    std::cout << "backend message sent\n";
    char buffer[BUFFER_SIZE];
    size_t bytes_read;                 
    std::string response = "";
    do {
        bytes_read = recv(backend_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        send(client_socket, buffer, bytes_read, 0);
        response.append(buffer, bytes_read);
    } while (bytes_read > 0);

    std::cout << "backend message received\n";
    close(backend_socket);
}