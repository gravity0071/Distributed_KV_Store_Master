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
#define HEARTBEAT_TIMEOUT 5 // 心跳超时时间（秒）
#define MONITOR_INTERVAL 2 // 监控线程检查间隔（秒）

std::mutex HeartbeatThread::monitorMutex;
bool HeartbeatThread::isMonitoring = false;

void HeartbeatThread::operator()() {
//    std::thread(&HeartbeatThread::monitorHeartbeats, this).detach();
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        if (!isMonitoring) {
            isMonitoring = true;
            std::thread(&HeartbeatThread::monitorHeartbeats, this).detach();
        }
    }
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

        try {
            auto json_data = nlohmann::json::parse(buffer);
            std::string operation = json_data["operation"];
            std::string storeId = json_data["storeId"];

            if (operation == "heartbeat") {

                kvStore.setStoreStatus(storeId,"true");

                std::cout << "Set heartbeat of client-" << storeId ;
                std::cout << " to " << kvStore.getStoreStatus(storeId) << ".\n";

                auto now = std::chrono::system_clock::now();
                std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
                std::ostringstream time_stream;
                time_stream << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
                kvStore.setLastHeartbeat(storeId,time_stream.str());

                std::cout << "Set lastHeartbeat of client-" << storeId ;
                std::cout << " to " << kvStore.getLastHeartbeat(storeId) << ".\n";

            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << "\n";
        }

    }

    std::cout << "Stopped handling client connection.\n";
}

//// monitor heartbeat status
void HeartbeatThread::monitorHeartbeats() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(MONITOR_INTERVAL));
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

        auto storeIds = kvStore.getAllStoreIds();
        for (const auto& storeId : storeIds) {
            std::tm last_heartbeat_tm = {};
            std::string lastHeartbeat = kvStore.getLastHeartbeat(storeId);
            std::istringstream time_stream(lastHeartbeat);
            time_stream >> std::get_time(&last_heartbeat_tm, "%Y-%m-%d %H:%M:%S");

            if (time_stream.fail()) {
                std::cerr << "Failed to parse last_heartbeat for storeId: " << storeId << "\n";
                continue;
            }

            auto last_heartbeat_time_t = std::mktime(&last_heartbeat_tm);
            auto last_heartbeat_tp = std::chrono::system_clock::from_time_t(last_heartbeat_time_t);

            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_tp).count() > HEARTBEAT_TIMEOUT) {
                std::string alive = kvStore.getStoreStatus(storeId);
                if(alive == "true"){
                    kvStore.setStoreStatus(storeId,"false");
                    std::cout << "Set heartbeat of client-" << storeId ;
                    std::cout << " to " << kvStore.getStoreStatus(storeId) << ".\n";
                }
            }
        }
    }
}

void HeartbeatThread::acceptServerConnections() {
    Server server(HEART_PORT);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> heartBeat_threads;

    while (true ) {
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