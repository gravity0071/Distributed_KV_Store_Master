#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// The Server class provides a basic TCP server implementation
// that can initialize, accept connections, and manage client connections.
class Server {
public:
    // Constructor that accepts a port number and initializes the server's address structure.
    Server(int port);

    // Destructor to ensure that the server socket is closed properly when the object is destroyed.
    ~Server();

    // Initializes the server by creating, configuring, binding, and listening on the server socket.
    // Returns true on success, false on failure.
    bool initialize();

    // Accepts an incoming client connection.
    // Returns a new socket descriptor for the client, or -1 if the connection fails.
    int acceptConnection();

    // Closes the connection with a specific client, given the client's socket descriptor.
    void closeConnection(int client_socket);

    // Closes the server socket, releasing any resources associated with it.
    void closeServer();

private:
    int port_;                 // Port number the server listens on.
    int server_fd_;            // Server socket file descriptor.
    struct sockaddr_in address_; // Structure holding server address information.
    int addrlen_;              // Length of the server address structure.

    // Configures the server socket with necessary options.
    // Returns true on success, false on failure.
    bool configureSocket();
};

#endif // SERVER_H