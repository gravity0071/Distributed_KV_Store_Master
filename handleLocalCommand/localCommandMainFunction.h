#ifndef COMMAND_THREAD_H
#define COMMAND_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/SharedStringVector.h"
#define COMMAND_PORT 8082

class CommandThread {
private:
    KVStoreMap& kvStore;
    ConsistentHashingMap& consistentMap;
    SharedStringVector& sharedVector;
    JsonParser& jsonParser;

public:
    CommandThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, JsonParser& jsonParser, SharedStringVector& sharedVector)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser), sharedVector(sharedVector) {}

    void operator()();
    void acceptServerConnections();
    void handleServer(int command_socket);
};

#endif // COMMAND_THREAD_H