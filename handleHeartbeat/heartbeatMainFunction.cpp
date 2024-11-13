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
    // Receive the client's JSON data, stored in buffer
//    char buffer[1024] = {0};
//    int bytes_read = recv(heartbeat_socket, buffer, sizeof(buffer) - 1, 0); // Leave space for null termination
//    std::cout << "handling" << std::endl;

//    std::string it = kvStore.getIp("1");
//    std::cout << it << std::endl;

//todo: use recv() to receive message from the servers, use send() to send message to the kv servers
    // Send the ACK/NACK to client
//    send(heartbeat_socket, it.c_str(), it.size(), 0);

    close(heartbeat_socket); // Close the client socket after communication
//    std::cout << "Closed connection with client" <<std::endl;
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