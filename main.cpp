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
    //
    //
    HeartbeatThread heartbeatThread(consistentMap);
    CommandThread commandThread(kvStore, consistentMap, sharedVector); //8082

    std::thread client(clientThread);
    std::thread heartbeat(heartbeatThread);
    std::thread command(commandThread);

    client.join();
    heartbeat.join();
    command.join();

//    std::cout << kvStore.read("client_key") << std::endl;
//    std::cout << consistentMap.read(1) << std::endl;

    return 0;
}