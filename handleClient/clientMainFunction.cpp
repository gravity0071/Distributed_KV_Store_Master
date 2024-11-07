//
// Created by Shawn Wan on 2024/11/6.
//
#include "clientMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <utility>

void ClientThread::operator()() {
    connectToClient();
}

// Function to handle communication with a single client
//todo implement the corresponding function here, using kvStore & consistentMap to achieve the corresponding function
void ClientThread::handleClient(int client_socket) {
    //recieve the client's json data, store in buffer
    char buffer[1024] = {0};
    int bytes_read = recv(client_socket, buffer, 1024, 0);
    if (bytes_read > 0) {
        std::cout << "Received from client: " << buffer << std::endl;
    }

    //the main function of the first step of query: client query master for the destination of the store server
    std::map clientLookUpMap = jsonParser.JsonToMap(buffer);
    std::string key = "NULL";
    if (clientLookUpMap.find("operation") != clientLookUpMap.end() ||
            clientLookUpMap.find("key") != clientLookUpMap.end()) {
        key = clientLookUpMap["key"];
    } else {
        std::cout << "Key 'operation' not found in map." << std::endl;
        //todo: may need to return fail to client
    }

    //send the ACK/NACK to client
    send(client_socket, key.c_str(), key.size(), 0);

    close(client_socket);  // Close the client socket after communication
    std::cout << "Closed connection with client" << std::endl;
}

// initiate multiple threads to multiple clients
void ClientThread::connectToClient() {
    Server server(PORT);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> client_threads;

    while (true) {
        // Accept a new client connection
        int client_socket = server.acceptConnection();
        if (client_socket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "Accepted new client connection." << std::endl;

        // Spawn a new thread to handle the client
        client_threads.emplace_back(&ClientThread::handleClient, this, client_socket); // Corrected to &ClientThread::handleClient
    }

    // Join all client threads (optional, depending on whether you want to wait for all threads to finish)
    for (auto& t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server when done (though in this loop, it runs indefinitely)
    server.closeServer();
}


