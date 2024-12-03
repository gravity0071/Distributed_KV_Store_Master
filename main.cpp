#include "handleClient/clientMainFunction.h"
#include "handleHeartbeat/heartbeatMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "util/KVStoreMap.h"
#include "util/ConsistentHashingMap.h"
#include "util/SharedStringVector.h"
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>


int main() {
    KVStoreMap kvStore;
    ConsistentHashingMap consistentMap;
    SharedStringVector sharedVector;
    JsonParser jsonParser;
    TcpConnectionUtility tcpConnectionUtility;

    {
        CommandThread CommandThread1(kvStore, consistentMap, jsonParser, sharedVector);
        int times = 10;
        while (!CommandThread1.startNewServerAndSetNewPortsintoTmp(true)) {
            if (times == 0) {
                std::cout << "MainThread: initiate a new Server failed, exiting system" << std::endl;
                return 0;
            }
            times--;
        }
    }


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

    std::map<std::string, std::string> shutDownMap;
    shutDownMap["operation"] = "close";
    for (const auto& store : kvStore) {
        const auto& key = store.first;
        shutDownMap["storeId"] = key;
        std::string sendString = jsonParser.MapToJson(shutDownMap);
        int socket = tcpConnectionUtility.connectToServer(kvStore.getIp(key), std::stoi(kvStore.getAddDropPort(key)));
        kvStore.deleteData(key);
        if(socket < 0) {
            std::cerr << "MainThread: " << key << " shut down error, need manually shut down" << std::endl;
            continue;
        }
        if (send(socket, sendString.c_str(), sendString.size(), 0) < 0) {
            std::cerr << "MainThread: Error sending shutdown message to " << key << std::endl;
            continue;
        }
        close(socket);
    }
    return 0;
}