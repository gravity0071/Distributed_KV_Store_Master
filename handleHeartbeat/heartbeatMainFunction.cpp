//
// Created by Shawn Wan on 2024/11/6.
//
#include "heartbeatMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <utility>

void HeartbeatThread::operator()() {
    acceptServerConnections();
}

//todo: implement the corresponding function here, using kvStore & consistentMap to achieve the corresponding function
void HeartbeatThread::handleServer(int heartbeat_socket) {
    char buffer[1024]; // Buffer to hold the received data

    // Loop to continuously receive and process messages
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer for each new message

        int bytes_read = recv(heartbeat_socket, buffer, sizeof(buffer) - 1, 0); // Leave space for null termination

        if (bytes_read < 0) {
            // Handle error in receiving data
            perror("Error reading from socket");
            break; // Exit the loop on error
        } else if (bytes_read == 0) {
            // Handle client disconnect
            std::cout << "Client disconnected.\n";
            break; // Exit the loop on disconnect
        }

        buffer[bytes_read] = '\0'; // Null-terminate the buffer to safely print as a string

        // Print the received data
        std::cout << "Received from client: " << buffer << std::endl;
    }

    std::cout << "Stopped handling client connection.\n";
}

void HeartbeatThread::acceptServerConnections() {
    Server server(HEART_PORT);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> heartBeat_threads;

    while (true) {
        // Accept a new client connection
        int heartbeat_socket = server.acceptConnection();
        if (heartbeat_socket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

//        std::cout << "Accepted new client connection." << std::endl;

        // Spawn a new thread to handle the client
        heartBeat_threads.emplace_back(&HeartbeatThread::handleServer, this,
                                       heartbeat_socket); // Corrected to &ClientThread::handleClient
    }

    // Join all client threads (optional, depending on whether you want to wait for all threads to finish)
    for (auto &t: heartBeat_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server when done (though in this loop, it runs indefinitely)
    server.closeServer();
}