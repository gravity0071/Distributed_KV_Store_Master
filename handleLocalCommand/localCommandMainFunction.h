#ifndef COMMAND_THREAD_H
#define COMMAND_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/SharedStringVector.h"
#include "../util/TcpConnectionUtility.h"
#define COMMAND_PORT 8082

class CommandThread {
private:
    KVStoreMap& kvStore;
    ConsistentHashingMap& consistentMap;
    SharedStringVector& sharedVector;
    JsonParser& jsonParser;
    TcpConnectionUtility& tcpConnectionUtility;
    int migratingStoreSocket1;
    int migratingStoreSocket2;

public:
    CommandThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, JsonParser& jsonParser, SharedStringVector& sharedVector)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser),
            sharedVector(sharedVector), tcpConnectionUtility(tcpConnectionUtility) {}

    void operator()();
    int initConnection();
    void handleServer();
};

#endif // COMMAND_THREAD_H