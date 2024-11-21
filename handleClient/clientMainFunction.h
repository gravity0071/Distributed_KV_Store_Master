#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/JsonParser.h"
#include <string>

#define CLIENT_PORT 8080 // 定义 Master 监听的端口

class ClientThread {
private:
    KVStoreMap& kvStore;                 // 用于存储键值映射
    ConsistentHashingMap& consistentMap; // 用于一致性哈希的映射
    JsonParser& jsonParser;              // 用于解析和构造 JSON 数据

public:
    ClientThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, JsonParser& jsonParser)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser) {}

    void operator()();              // 线程入口
    void connectToClient();         // 处理客户端连接
    void handleClient(int clientSocket); // 处理单个客户端请求
};

void initializeHashMap(ConsistentHashingMap& consistentMap);

#endif // CLIENT_THREAD_H
