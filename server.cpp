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

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "127.0.0.1"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const std::string& path);
void proxy_remote_file(struct server_app *app, int client_socket, const std::string& request);

// The main function is provided and no change is needed
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
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
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

std::string get_file_extension(const std::string& str) {
    for (auto it = str.end(); it >= str.begin(); it--) {
        if (*it == '.') {
            return std::string(it+1, str.end());
            break;
        }
    }
    return "";
}

std::string url_decode(const std::string &value) {
    std::string decoded;
    std::stringstream ss(value);

    while (!ss.eof()) {
        int hex;
        char c;
        
        // Read characters until encountering '%'
        while (ss.peek() != '%' && !ss.eof()) {
            decoded += ss.get();
        }

        if (ss.peek() == '%') {
            // Skip '%'
            ss.ignore();

            // Read two hexadecimal characters
            ss >> std::hex >> hex;
            
            // Convert hexadecimal value to character
            c = static_cast<char>(hex);
            
            // Add character to decoded string
            decoded += c;
        }
    }

    return decoded;
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    std::string request(buffer);

    std::vector<std::string> requestTokens = split(request, "\r\n");
    std::vector<std::string> requestLineTokens = split(requestTokens[0], " ");

    for (auto token : requestTokens) {
        std::cout << token << "\n";
    }

    std::cout << "remote host: " << app->remote_host << "\n";
    std::cout << app->remote_port << "\n";
    std::cout << app->server_port << "\n";

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    std::string requestURI = requestLineTokens[1];
    std::cout << requestURI << '\n';
    requestURI.erase(0,1);
    // size_t pos = requestURI.find("%20");
    // while (pos != std::string::npos) {
    //     requestURI.replace(pos, 3, " ");
    //     pos = requestURI.find("%20");
    // }
    // pos = requestURI.find("%");
    // while (pos != std::string::npos && pos < requestURI.length() - 2) {

    // }
    requestURI = url_decode(requestURI);
    std::cout << requestURI << '\n';
    std::string file_name = requestURI == "" ? "index.html" : requestURI;

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    std::string ext = get_file_extension(file_name);
    if (ext == "ts") {
        proxy_remote_file(app, client_socket, request);
    }
    else {
        serve_local_file(client_socket, file_name);
    }
    //}
}

void serve_local_file(int client_socket, const std::string& path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    // get file extension
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
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    int backend_socket;
    if ((backend_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "socket creation error\n";
        char response[] = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(app->remote_port);

    if (inet_pton(AF_INET, app->remote_host, &backend_addr.sin_addr) <= 0) {
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
    // ssize_t bytes_read;
    // if ((bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) <= 0) {
    //     std::cout << "error receiving message from backend\n";
    //     char response[] = "HTTP/1.0 502 Bad Gateway \r\n\r\n";
    //     send(client_socket, response, strlen(response), 0);
    //     return;
    // }
    // buffer[bytes_read] = '\0';
    //std::cout << "client message:\n";
    //std::cout << response;

    // size_t length = response.length();
    // char cstr[length];
    // char *ptr = cstr;
    // strcpy(ptr, response.c_str());
    // while (length > 0) {
    //     int i = send(client_socket, ptr, length, 0);
    //     if (i < 1) exit(1);
    //     ptr += i;
    //     length -= i;
    // }

    //send(client_socket, response.c_str(), response.length(), 0);
}