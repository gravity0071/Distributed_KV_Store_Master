#ifndef COMMAND_THREAD_H
#define COMMAND_THREAD_H

#include "../util/KVStoreMap.h"
#include "../util/ConsistentHashingMap.h"
#include "../util/SharedStringVector.h"
#include "../util/TcpConnectionUtility.h"

#define MASTER_IP "127.0.0.1"
#define INITATE_COMMAND_ADDRESS "/Users/shawnwan/Documents/cpp/Distributed_KV_Store_Server/build/MyExecutable" // store_test 127.0.0.1 8081
#define HEARTBEAT_PORT "8081"
#define KV_STORE_SERVER_IP "127.0.0.1"
#define HASH_KEY_RANGE 10000

class CommandThread {
private:
    KVStoreMap &kvStore;
    ConsistentHashingMap &consistentMap;
    SharedStringVector &sharedVector;
    JsonParser &jsonParser;
    TcpConnectionUtility &tcpConnectionUtility;

    int migratingStoreSocketSend = -1;
    int migratingStoreSocketRecv = -1;
    std::string sourceServerId = "-1";
    std::string destServerId = "-1";

    int newServerClientPort = -1;
    int newServerCommandPort = -1;
    std::string newServerId = "";
    std::string movingKeyRange = "";

public:
    CommandThread(KVStoreMap &kvStore, ConsistentHashingMap &consistentMap, JsonParser &jsonParser,
                  SharedStringVector &sharedVector)
            : kvStore(kvStore), consistentMap(consistentMap), jsonParser(jsonParser),
              sharedVector(sharedVector), tcpConnectionUtility(tcpConnectionUtility) {}

    void operator()();

    int initConnection(const std::string &serverIp, int serverPort);

    void handleServer();

    int addServer();

    int removeServer();

    void testAddServerIntoKvStore();

    std::string checkWhichServerValidRemoving(std::string &leftAdjacent, std::string &rightAdjacent);
    std::string checkWhichServerMoveFrom();

    int buildConnect(std::string &server1, std::string &server2); //build tcp connection to two servers and bind them to migratingStoreSocket1,2
    //data flow:  1->2

    void closeAllConnection();

    int receiveAndSendDataBetweenServers(bool isAdd);

    int handleSendServer(bool isDelete);

    int handleReceiveServer();

    int sendSourIpPortToDes();

    std::map<std::string, std::string> getIpandPortFromSource();

    int sendIpPorttoRecv(std::string &sourceIp, int sourcePort);

    int receiveData(int clientSocket, std::string &receivedData);

    void operationAfterRemovingOrAdding(bool isAdd);

    int startNewServerAndSetNewPortsintoTmp(bool isTheFirstServer);
};

#endif // COMMAND_THREAD_H