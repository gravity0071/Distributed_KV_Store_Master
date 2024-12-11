#include "clientMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <optional>
#include <unistd.h> // for close()

void ClientThread::operator()() {
    connectToClient();
}

void ClientThread::handleClient(int clientSocket) {
    while (true) {
        char buffer[1024] = {0};
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
//            std::cerr << "Failed to receive data from client or client disconnected." << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';
//        std::cout << "Received query from client: " << buffer << std::endl;

        std::map<std::string, std::string> clientRequest = jsonParser.JsonToMap(buffer);
        if (clientRequest.find("operation") == clientRequest.end() ||
            clientRequest.find("key") == clientRequest.end()) {
            std::cerr << "Invalid request format." << std::endl;
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid request format"}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
            continue;
        }

        std::string operation = clientRequest["operation"];
        std::string key = clientRequest["key"];

        if (operation == "lookup") {
            std::string storeId = consistentMap.findParticularKey(key);
            if (!storeId.empty()) {
                std::string storeStatus = kvStore.getStoreStatus(storeId);
                if (storeStatus == "true") {
                    std::string ip = kvStore.getIp(storeId);
                    std::string port = kvStore.getClientPort(storeId);

                    std::string jsonResponse = jsonParser.MapToJson({
                                                                            {"store_id", storeId},
                                                                            {"key",      key},
                                                                            {"ip",       ip},
                                                                            {"port",     port}
                                                                    });
                    send(clientSocket, jsonResponse.c_str(), jsonResponse.size(), 0);
//                    std::cout << "Sent response to client: " << jsonResponse << std::endl;
                } else {
                    std::string errorResponse = jsonParser.MapToJson({
                                                                             {"error",    "Store is not alive"},
                                                                             {"store_id", storeId}
                                                                     });
                    send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
                }
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found"}});
                send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "exit") {
//            std::cout << "Client requested to exit." << std::endl;
            break;
        } else {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Unsupported operation"}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
    }

    close(clientSocket);
//    std::cout << "Connection with client closed." << std::endl;
}

void ClientThread::connectToClient() {
    Server server(CLIENT_PORT);

    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> clientThreads;

    while (true) {
        int clientSocket = server.acceptConnection();
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

//        std::cout << "Accepted new client connection." << std::endl;

        clientThreads.emplace_back(&ClientThread::handleClient, this, clientSocket);
    }
}