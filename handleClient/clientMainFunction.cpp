#include "clientMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <utility>
#include <unistd.h> // for close()

void ClientThread::operator()() {
    connectToClient();
}

// 处理单个客户端请求，支持多次查询
void ClientThread::handleClient(int clientSocket) {
    while (true) {
        char buffer[1024] = {0};
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            std::cerr << "Failed to receive data from client or client disconnected." << std::endl;
            break; // 退出循环，关闭连接
        }

        buffer[bytesRead] = '\0'; // 确保字符串以 NULL 结尾
        std::cout << "Received query from client: " << buffer << std::endl;

        // 解析客户端请求
        std::map<std::string, std::string> clientRequest = jsonParser.JsonToMap(buffer);
        if (clientRequest.find("operation") == clientRequest.end() || clientRequest.find("key") == clientRequest.end()) {
            std::cerr << "Invalid request format." << std::endl;
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid request format"}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
            continue; // 等待下一次请求
        }

        std::string operation = clientRequest["operation"];
        std::string key = clientRequest["key"];

        if (operation == "lookup") {
            // 查询 key 对应的 storeId
            std::string storeId = consistentMap.findParticularKey(key);
            if (!storeId.empty()) {
                // 模拟从存储节点获取值（在实际应用中，这里可以查询存储节点）
                std::string value = "Value_For_" + key; // 模拟的查询值

                // 构造 JSON 响应
                std::string jsonResponse = jsonParser.MapToJson({
                                                                        {"store_id", storeId},
                                                                        {"key", key},
                                                                        {"value", value} // 返回查询的值
                                                                });
                send(clientSocket, jsonResponse.c_str(), jsonResponse.size(), 0);
                std::cout << "Sent response to client: " << jsonResponse << std::endl;
            } else {
                // 如果未找到 key，则返回错误
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found"}});
                send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "exit") {
            // 如果收到退出指令，结束循环
            std::cout << "Client requested to exit." << std::endl;
            break;
        } else {
            // 不支持的操作类型
            std::string errorResponse = jsonParser.MapToJson({{"error", "Unsupported operation"}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
    }

    close(clientSocket); // 关闭客户端连接
    std::cout << "Connection with client closed." << std::endl;
}

// 初始化服务器并处理客户端连接
void ClientThread::connectToClient() {
    Server server(CLIENT_PORT);

    initializeHashMap(consistentMap);

    // 初始化服务器
    if (!server.initialize()) {
        std::cerr << "Server initialization failed." << std::endl;
        return;
    }

    std::vector<std::thread> clientThreads;

    while (true) {
        // 接受新的客户端连接
        int clientSocket = server.acceptConnection();
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "Accepted new client connection." << std::endl;

        // 启动一个线程处理客户端
        clientThreads.emplace_back(&ClientThread::handleClient, this, clientSocket);
    }

    // 可选：等待所有线程完成
    for (auto& t : clientThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    server.closeServer();
}

// 初始化哈希环（仅用于调试）
void initializeHashMap(ConsistentHashingMap& consistentMap) {
    consistentMap.addNew("0-20000", "0-10000", "store1");
    consistentMap.addNew("10000-20000", "10000-20000", "store2");
    consistentMap.addNew("20000-65536", "20000-65536", "store3");
}

