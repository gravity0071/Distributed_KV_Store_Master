//
// Created by Shawn Wan on 2024/11/7.
//
#include "gtest/gtest.h"
#include "../util/Server.h"
#include "../handleClient/clientMainFunction.h"
#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/JsonParser.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>

// Test fixture for ClientThread
class ClientThreadTest : public ::testing::Test {
protected:
    KVStoreMap kvStore;
    ConsistentHashingMap consistentMap;
    JsonParser jsonParser;
    ClientThread clientThread;
    std::thread serverThread;

    ClientThreadTest()
            : clientThread(kvStore, consistentMap, jsonParser) {}

    // Set up server in a separate thread
    void SetUp() override {
        serverThread = std::thread([this]() {
            Server server(PORT);
            if (server.initialize()) {
                while (true) {
                    int client_socket = server.acceptConnection();
                    if (client_socket >= 0) {
                        std::thread(&ClientThread::handleClient, clientThread, client_socket).detach();
                    }
                }
            }
        });
        // Give the server a moment to initialize before running tests
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (serverThread.joinable()) {
            serverThread.detach();  // Detach the server thread as it runs indefinitely
        }
    }

    // Function to create a client socket, connect to the server, and send data
    int createClientSocketAndSendRequest(const std::string& request) {
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_TRUE(client_socket >= 0);

        struct sockaddr_in server_address {};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(PORT);
        server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        // Connect to server
        int result = connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
        EXPECT_EQ(result, 0);

        // Send the request to the server
        send(client_socket, request.c_str(), request.size(), 0);

        return client_socket;
    }

    // Function to receive the response from the server
    std::string receiveResponse(int client_socket) {
        char buffer[1024] = {0};
        int bytes_received = recv(client_socket, buffer, 1024, 0);
        EXPECT_GT(bytes_received, 0);
        close(client_socket);
        return std::string(buffer, bytes_received);
    }
};

// Test case to verify multiple clients can connect and receive correct responses concurrently
TEST_F(ClientThreadTest, HandlesMultipleClientConnections) {
    // Set up different keys and responses in KVStoreMap
    kvStore.write("key1", R"({"ip": "192.168.1.1", "port": "8080"})");
    kvStore.write("key2", R"({"ip": "192.168.1.2", "port": "8181"})");

    std::string request1 = R"({"operation": "LOOKUP", "key": "key1"})";
    std::string request2 = R"({"operation": "LOOKUP", "key": "key2"})";

    // Create multiple client connections
    std::vector<std::thread> clientThreads;
    std::vector<std::string> responses(2);

    // Client 1
    clientThreads.emplace_back([&]() {
        int client_socket1 = createClientSocketAndSendRequest(request1);
        responses[0] = receiveResponse(client_socket1);
    });

    // Client 2
    clientThreads.emplace_back([&]() {
        int client_socket2 = createClientSocketAndSendRequest(request2);
        responses[1] = receiveResponse(client_socket2);
    });

    // Wait for all clients to complete
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Verify that each client received the expected response
    EXPECT_EQ(responses[0], "key1");  // Expected response for client 1
    EXPECT_EQ(responses[1], "key2");  // Expected response for client 2
}