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

// 处理单个客户端请求，支持多次查询
void ClientThread::handleClient(int clientSocket) {
    while (true) {
        // 测试
        kvStore.setAllFields("store1", "127.0.0.1", "8001", "8002", "8085", "true", "100", "0-10000", "store0", "store2");
        kvStore.setAllFields("store2", "127.0.0.1", "8001", "8002", "8085", "true", "50", "10000-20000", "store1", "store3");
        kvStore.setAllFields("store3", "127.0.0.1", "8001", "8002", "8085", "false", "30", "20000-30000", "store2", "store1");
        //
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
                // 检查 store 是否存活
                std::string storeStatus = kvStore.getStoreStatus(storeId);
                if (storeStatus == "true") {
                    // 构造响应信息，返回存储节点的 IP 和 Port
                    std::string ip = kvStore.getIp(storeId);
                    std::string port = kvStore.getClientPort(storeId);

                    std::string jsonResponse = jsonParser.MapToJson({
                                                                            {"store_id", storeId},
                                                                            {"key", key},
                                                                            {"ip", ip},
                                                                            {"port", port}
                                                                    });
                    send(clientSocket, jsonResponse.c_str(), jsonResponse.size(), 0);
                    std::cout << "Sent response to client: " << jsonResponse << std::endl;
                } else {
                    // 返回存储节点不可用的错误
                    std::string errorResponse = jsonParser.MapToJson({
                                                                             {"error", "Store is not alive"},
                                                                             {"store_id", storeId}
                                                                     });
                    send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
                }
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
