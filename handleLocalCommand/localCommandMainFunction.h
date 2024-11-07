#ifndef COMMAND_THREAD_H
#define COMMAND_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/SharedStringVector.h"

class CommandThread {
private:
    KVStoreMap& kvStore;
    ConsistentHashingMap& consistentMap;
    SharedStringVector& sharedVector;

public:
    CommandThread(KVStoreMap& kvStore, ConsistentHashingMap& consistentMap, SharedStringVector& sharedVector)
            : kvStore(kvStore), consistentMap(consistentMap), sharedVector(sharedVector) {}

    void operator()() {
        kvStore.write("command_key", "command_value");
        consistentMap.write(2, "command_data");
        sharedVector.add("Command processed");
    }
};

#endif // COMMAND_THREAD_H