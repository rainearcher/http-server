#ifndef SERVER_H
#define SERVER_H
#include <string>
#include "smartSocket.h"

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

#endif