#ifndef HEARTBEAT_THREAD_H
#define HEARTBEAT_THREAD_H

#include "../util/ConsistentHashingMap.h"

class HeartbeatThread {
private:
    ConsistentHashingMap& consistentMap;

public:
    HeartbeatThread(ConsistentHashingMap& consistentMap)
            : consistentMap(consistentMap) {}

    void operator()() {
        consistentMap.write(1, "heartbeat_value");
    }
};

#endif // HEARTBEAT_THREAD_H