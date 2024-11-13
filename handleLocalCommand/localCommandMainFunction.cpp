//
// Created by Shawn Wan on 2024/11/6.
//

#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <utility>

void CommandThread::operator()() {
    acceptServerConnections();
}

void CommandThread::handleServer(int command_socket) {
    char buffer[1024] = {0};

    kvStore.setIp("1", "1");
    kvStore.setIp("1", "2");

    close(command_socket); // Close the client socket
    std::cout << "Closed connection with client" << std::endl;
}

void CommandThread::acceptServerConnections() {
    Server server(COMMAND_PORT);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> command_threads;

    while (true) {
        // Accept a new client connection
        int command_socket = server.acceptConnection();
        if (command_socket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "Accepted new client connection." << std::endl;

        // Spawn a new thread to handle the client
        command_threads.emplace_back(&CommandThread::handleServer, this,
                                     command_socket); // Corrected to &ClientThread::handleClient
    }

    // Join all client threads (optional, depending on whether you want to wait for all threads to finish)
    for (auto &t: command_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server when done (though in this loop, it runs indefinitely)
    server.closeServer();
}