#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/JsonParser.h"

#define PORT 8080


class ClientThread {
private:
    KVStoreMap& kvStore;
    ConsistentHashingMap& consistentMap;
    JsonParser& jsonParser;

public:
    ClientThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, JsonParser& jsonParser)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser){}

    void operator()();
    void connectToClient();
    void handleClient(int clientSocket);
};

#endif // CLIENT_THREAD_H