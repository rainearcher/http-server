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
#include <exception>
#include "smartSocket.h"

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
void handle_request(struct server_app *app, const SmartSocket& client_socket);
void serve_local_file(const SmartSocket& socket, const std::string& path);
void try_proxy_remote_file(struct server_app *app, const SmartSocket& client_socket, const std::string& request);

std::string receive_client_request(const SmartSocket& socket);
std::string parse_filename_from_request(const std::string& request);
std::string decode_uri(const std::string &value);
std::string get_file_extension(const std::string& str);

std::string read_file_content(std::fstream& file);
std::string get_content_type(const std::string& path);
std::string format_response(const std::string& code, const std::string& contentType, const std::string& content);
std::string format_ok_response(const std::string& contentType, const std::string& content);
void send_404_response(const SmartSocket& socket);
void send_file(const SmartSocket& socket, std::fstream& file, const std::string& contentType);

void send_502_response(const SmartSocket& socket);
void forward_backend_response_to_client(const SmartSocket& backend_socket, const SmartSocket& client_socket);
void inet_pton_throws_invalid_argument(int __af, const char *__restrict__ __cp, void *__restrict__ __buf);
void proxy_remote_file(struct server_app *app, const SmartSocket& client_socket, const std::string& request);
struct sockaddr_in init_backend_addr(struct server_app * app);
void connect_backend_socket_to_remote_host(struct server_app *app, const SmartSocket& backend_socket);
void forward_client_request_to_backend(const std::string& request, const SmartSocket& backend_socket);

int main(int argc, char *argv[])
{
    struct server_app app;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    SmartSocket server_socket = SmartSocket(socket(AF_INET, SOCK_STREAM, 0));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    server_socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (server_socket.bind((struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (server_socket.listen(10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        try {
            SmartSocket client_socket = SmartSocket(server_socket.accept((struct sockaddr*)&client_addr, &client_len));
        
            printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            handle_request(&app, client_socket);

        }
        catch (std::bad_alloc& e) {
            perror("accept failed");
            continue;
        }
    }

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

void handle_request(struct server_app *app, const SmartSocket& client_socket) {
    std::string request = receive_client_request(client_socket);
    std::string filename = parse_filename_from_request(request);

    if (get_file_extension(filename) == "ts") {
        try_proxy_remote_file(app, client_socket, request);
    }
    else {
        serve_local_file(client_socket, filename);
    }
}

std::string receive_client_request(const SmartSocket& socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = socket.recv(buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {// Connection closed or error
        throw std::runtime_error("client connection closed or recv error");
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

void serve_local_file(const SmartSocket& client_socket, const std::string& path) {
    std::fstream file(path);
    if (!file.good()) {
        send_404_response(client_socket);
    }
    else {
        send_file(client_socket, file, get_content_type(path));
    }
}

void send_file(const SmartSocket& client_socket, std::fstream& file, const std::string& contentType) {
    std::string content = read_file_content(file);
    std::string response = format_ok_response(contentType, content);
    client_socket.send(response.c_str(), response.length(), 0);
}

void send_404_response(const SmartSocket& client_socket) {
    std::string response = format_response("404 Not Found", "text/plain; charseet=UTF-8", "File not found");
    client_socket.send(response.c_str(), response.length(), 0);
}

std::string read_file_content(std::fstream& file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
}

std::string format_ok_response(const std::string& contentType, const std::string& content) {
    return format_response("200 OK", contentType, content);
}

std::string format_response(const std::string& code, const std::string& contentType, const std::string& content) {
    std::string response = "HTTP/1.0 " + code + "\r\n" 
                            + "Content-Type: " + contentType + "\r\n"
                            + "Content-Length: " + std::to_string(content.length()) + "\r\n\r\n"
                            + content;
    return response;
}

std::string get_content_type(const std::string& path) {
    std::string extension = get_file_extension(path);
    if (extension == "html") {
        return "text/html; charset=UTF-8";
    }
    else if (extension == "txt") {
        return "text/plain; charset=UTF-8";
    }
    else if (extension == "jpg") {
        return "image/jpeg";
    }
    else {
        return "application/octet-stream";
    }
}

void try_proxy_remote_file(struct server_app *app, const SmartSocket& client_socket, const std::string& request) {
    try {
        proxy_remote_file(app, client_socket, request);
    } 
    catch (std::bad_alloc& e) {
        std::cout << "socket creation error\n";
        send_502_response(client_socket);
    }
    catch (std::exception& e) {
        std::cout << e.what();
        send_502_response(client_socket);
    }
}

void proxy_remote_file(struct server_app *app, const SmartSocket& client_socket, const std::string& request) {
    SmartSocket backend_socket = SmartSocket(socket(AF_INET, SOCK_STREAM, 0));
    connect_backend_socket_to_remote_host(app, backend_socket);
    forward_client_request_to_backend(request, backend_socket);
    forward_backend_response_to_client(backend_socket, client_socket);
}

void connect_backend_socket_to_remote_host(struct server_app *app, const SmartSocket& backend_socket) {
    struct sockaddr_in backend_addr = init_backend_addr(app);
    backend_socket.connect(reinterpret_cast<struct sockaddr*>(&backend_addr), sizeof(backend_addr));
}

struct sockaddr_in init_backend_addr(struct server_app * app) {
    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(app->backend_port);
    inet_pton_throws_invalid_argument(AF_INET, app->backend_host, &backend_addr.sin_addr);
    return backend_addr;
}

void inet_pton_throws_invalid_argument(int __af, const char *__restrict__ __cp, void *__restrict__ __buf) {
    if (inet_pton(__af, __cp, __buf) <= 0) {
        throw std::invalid_argument("invalid address/address not supported");
    }
}

void forward_client_request_to_backend(const std::string& request, const SmartSocket& backend_socket) {
    backend_socket.send(request.c_str(), request.length(), 0);
}

void forward_backend_response_to_client(const SmartSocket& backend_socket, const SmartSocket& client_socket) {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    do {
        bytes_read = backend_socket.recv(buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        client_socket.send(buffer, bytes_read, 0);
    } while (bytes_read > 0);
}

void send_502_response(const SmartSocket& socket) {
    std::string response = format_response("502 Bad Gateway", "text/plain", "");
    socket.send(response.c_str(), response.length(), 0);
}
