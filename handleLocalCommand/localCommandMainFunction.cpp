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
    if (initConnection() == -1) {
        return; // Exit if connection fails
    }
    handleServer();
}

int CommandThread::initConnection() {
    // Initialize connection to the server
    migratingStoreSocket1 = tcpConnectionUtility.connectToServer("127.0.0.1", 8085);
    if (migratingStoreSocket1 < 0) {
        perror("Socket creation failed");
        return -1;
    }
    return 0;
}

void CommandThread::handleServer() {
    char buffer[1024] = {0};

    while (true) {
        // Read input from the user
        std::cout << "Enter command: ";
        std::cin.getline(buffer, sizeof(buffer));

        // Send the command to the server
        if (send(migratingStoreSocket1, buffer, strlen(buffer), 0) < 0) {
            perror("Failed to send command");
            break; // Exit loop on send failure
        }

        // Exit loop if the user enters "close"
        if (std::string(buffer) == "close") {
            std::cout << "Exiting...\n";
            break;
        }
    }

    // Clean up socket
    close(migratingStoreSocket1);
    migratingStoreSocket1 = -1;
}