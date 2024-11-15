#include "handleClient/clientMainFunction.h"
#include "handleHeartbeat/heartbeatMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "util/KVStoreMap.h"
#include "util/ConsistentHashingMap.h"
#include "util/SharedStringVector.h"
#include <thread>

int main() {
    KVStoreMap kvStore;
    ConsistentHashingMap consistentMap;
    SharedStringVector sharedVector;
    JsonParser jsonParser;

    //8080,
    // kvStore: to check whether the store it want connected to is alive,
    // consistentMap: to look up which store the key is located
    //jsonParser: convert map to string or string to map
    ClientThread clientThread(kvStore, consistentMap, jsonParser);

    //8081
    HeartbeatThread heartbeatThread(kvStore, consistentMap, jsonParser, sharedVector);

    // 8082
    CommandThread commandThread(kvStore, consistentMap, jsonParser, sharedVector); //8082

    std::thread client(clientThread);
    std::thread heartbeat(heartbeatThread);
    std::thread command(commandThread);

    std::cout<<"master running" << std::endl;
    client.join();
    heartbeat.join();
    command.join();
    return 0;
}