#ifndef HEARTBEAT_THREAD_H
#define HEARTBEAT_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/SharedStringVector.h"
#include "../util/SharedStringVector.h"
#define HEART_PORT 8081

class HeartbeatThread {
private:
    KVStoreMap& kvStore;
    ConsistentHashingMap& consistentMap;
    SharedStringVector& sharedVector;
    JsonParser& jsonParser;

public:
    HeartbeatThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, JsonParser& jsonParser, SharedStringVector& sharedVector)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser), sharedVector(sharedVector) {}

    void operator()();
    void acceptServerConnections();
    void handleServer(int heartbeat_socket);
};

#endif // HEARTBEAT_THREAD_H