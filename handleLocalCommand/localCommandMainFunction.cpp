//
// Created by Shawn Wan on 2024/11/6.
//
#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <utility>
#include <future>
#include <iostream>
#include <unistd.h>

void CommandThread::testAddServerIntoKvStore() {
    kvStore.setAllFields("store1",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "8084",            // Add/Drop Port
                         "8083",            // Client Port
                         "true",            // Status
                         "3000",             // KeyNum
                         "0-3000",           // KeyRange
                         "store2",          // Left Store ID
                         "store3"           // Right Store ID);
    );
    kvStore.setAllFields("store2",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "8086",            // Add/Drop Port
                         "8085",            // Client Port
                         "true",            // Status
                         "5000",             // KeyNum
                         "3000-8000",         // KeyRange
                         "store3",          // Left Store ID
                         "store1"           // Right Store ID
    );
    kvStore.setAllFields("store3",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "9094",            // Add/Drop Port
                         "9095",            // Client Port
                         "true",            // Status
                         "2001",             // KeyNum
                         "8000-10001",         // KeyRange
                         "store1",          // Left Store ID
                         "store2"           // Right Store ID
    );
//    sharedVector.add("store2");
}

void CommandThread::operator()() {
    testAddServerIntoKvStore();
    handleServer();
}

int CommandThread::initConnection(const std::string &serverIp, int serverPort) {
    // Initialize connection to the server
    int mig = tcpConnectionUtility.connectToServer(serverIp, serverPort);
    if (mig < 0) {
        std::cerr << "Failed to connect to server at "
                  << serverIp << ":" << serverPort
                  << ". Error: " << strerror(errno) << std::endl;
        return -1;
    }
    return mig;
}

void CommandThread::handleServer() {
    while (true) {
        std::cin.clear();
        // Read input from the user
        std::cout << "Enter command: add/drop: " << std::endl;
        std::string command;
        std::getline(std::cin, command);

        if (command == "add") {
            addServer();
        } else if (command == "drop") {
            removeServer();
        } else {
            std::cout << "Please input valid input" << std::endl;
        }
    }
}

int CommandThread::addServer() {
    std::cout << "add" << std::endl;
    return 1;
}

int CommandThread::removeServer() {
    if (kvStore.size() == 1) {
        std::cerr << "only have one kv_store server, cannot remove" << std::endl;
        return 0;
    }
    kvStore.displayAllKeys();
    std::cout << "type in the server you want to drop, exit type -1" << std::endl;

    while (true) {
        char inputRemoveServer[1024] = {0};
        std::cin.getline(inputRemoveServer, sizeof(inputRemoveServer));
        std::string input(inputRemoveServer);
        if (input == "-1")
            return 0;
        bool storeInTheSys = kvStore.haveKey(input);
        bool storeIsMigrating = sharedVector.have(input);

        if (storeInTheSys && !storeIsMigrating) {
            sourceServerId = input;
            break;
        } else {
            if (!storeInTheSys)
                std::cout << "type in valid server id" << std::endl;
            else if (storeIsMigrating)
                std::cout << "current store is migrating, please try later" << std::endl;
        }
    }

    std::string leftAdjacent = kvStore.getLeftStoreId(sourceServerId);
    std::string rightAdjacent = kvStore.getRightStoreId(sourceServerId);
    destServerId = checkWhichServerValidRemoving(leftAdjacent,
                                                 rightAdjacent); //decide which server to add all the key to
    if (destServerId == "-1") {
        std::cerr << "assign destServerId error" << std::endl;
        return 0;
    }

    if (!buildConnect(sourceServerId, destServerId)) {
        closeAllConnection();
        std::cerr << "build connection to two servers failed" << std::endl;
        return 0;
    }
    if (!receiveAndSendDataBetweenServers(false)) {
        closeAllConnection();
        std::cerr << "communicating between servers error" << std::endl;
        return 0;
    }
    //todo: modify both maps and vector

    closeAllConnection();
    return 1;
}

int CommandThread::buildConnect(std::string &server1, std::string &server2) {
    std::string server1Ip = kvStore.getIp(server1);
    int server1Port = std::stoi(kvStore.getAddDropPort(server1));
    std::string server2Ip = kvStore.getIp(server2);
    int server2Port = std::stoi(kvStore.getAddDropPort(server2));

//    connect to two servers
    if ((migratingStoreSocketSend = initConnection(server1Ip, server1Port)) == -1) {
        std::cerr << "build connection to server1 failed" << std::endl;
        return 0; // Exit if connection fails
    }
    if ((migratingStoreSocketRecv = initConnection(server2Ip, server2Port)) == -1) {
        std::cerr << "build connection to server2 failed" << std::endl;
        return 0; // Exit if connection fails
    }
    return 1;
}

std::string CommandThread::checkWhichServerValidRemoving(std::string &leftAdjacent, std::string &rightAdjacent) {
    if (leftAdjacent == rightAdjacent) {
        if (sharedVector.have(leftAdjacent)) {
            std::cerr << "adjacent servers are migrating" << std::endl;
            return "-1";
        }
        return leftAdjacent;
    }
    bool rightIsMigrating = sharedVector.have(rightAdjacent);
    bool leftIsMigrating = sharedVector.have(leftAdjacent);
    if (rightIsMigrating && leftIsMigrating) {
        std::cerr << "adjacent servers are migrating" << std::endl;
        return "-1";
    } else if (rightIsMigrating)
        return leftAdjacent;
    else if (leftIsMigrating)
        return rightAdjacent;
    else {
        int leftServerKeyNum = std::stoi(kvStore.getStoreKeyNum(leftAdjacent));
        int rightServerKeyNum = std::stoi(kvStore.getStoreKeyNum(rightAdjacent));
        return leftServerKeyNum > rightServerKeyNum ? leftAdjacent : rightAdjacent;
    }
}

void CommandThread::closeAllConnection() {
    close(migratingStoreSocketSend);
    close(migratingStoreSocketRecv);
    migratingStoreSocketSend = -1;
    migratingStoreSocketRecv = -1;
    sourceServerId = "-1";
    destServerId = "-1";
}

int CommandThread::receiveAndSendDataBetweenServers(bool isAdd) {
    if (!sendSourIpPortToDes()) {
        std::cerr << "failed on the building connection between destination and source server" << std::endl;
        return 0;
    }

    std::future<int> resultSender = std::async(std::launch::async, &CommandThread::handleSendServer, this, isAdd);
    std::future<int> resultReceiver = std::async(std::launch::async, &CommandThread::handleReceiveServer, this);

    int sendResult = resultSender.get();
    int recvResult = resultReceiver.get();

    //todo: implement the function that modify consistentHashingMap, KVStoreMap and sharedStringVector
    if (sendResult == 1 && recvResult == 1) {
        kvStore.displayAllData();
        consistentMap.displayHashRing();
    }

    return (sendResult == 0 && recvResult == 0) ? 0 : 1;
}

int CommandThread::sendSourIpPortToDes() {
    //get the sender ip and port
    std::map<std::string, std::string> SourceipAddressPort = getIpandPortFromSource();
    if (SourceipAddressPort.find("error") != SourceipAddressPort.end()) {
        std::cerr << "getIpandPortFromSource error" << std::endl;
        return 0;
    }
    std::string sourceIp = "";
    int sourcePort = 0;
    if (SourceipAddressPort.find("sender_Ip") != SourceipAddressPort.end() &&
        SourceipAddressPort.find("sender_Port") != SourceipAddressPort.end()) {
        sourceIp = SourceipAddressPort["sender_Ip"];
        sourcePort = std::stoi(SourceipAddressPort["sender_Port"]);
    } else {
        std::cerr << "source Ip and port get error" << std::endl;
        return 0;
    }

    //send the ip address to the recv server
    if (!sendIpPorttoRecv(sourceIp, sourcePort)) {
        std::cerr << "sendIpPrttoRecv error" << std::endl;
        return 0;
    }

    return 1;
}

int CommandThread::sendIpPorttoRecv(std::string &sourceIp, int sourcePort) {
    std::map<std::string, std::string> step3;
    step3["operation"] = "recv";
    step3["storeId"] = destServerId;
    step3["keyRange"] = kvStore.getKeyRange(sourceServerId);
    step3["sourceIp"] = kvStore.getIp(sourceServerId);
    step3["sourcePort"] = std::to_string(sourcePort);

    std::string step3_message = jsonParser.MapToJson(step3);

    if (send(migratingStoreSocketRecv, step3_message.c_str(), step3_message.size(), 0) < 0) {
        std::cerr << "message sending to data receiver error" << std::endl;
        return 0;
    }

    std::string buffer = "";
    if (!receiveData(migratingStoreSocketRecv, buffer))
        return 0;

    std::map<std::string, std::string> res = jsonParser.JsonToMap(buffer);
    if (res.find("operation") != res.end() && res["operation"] == "connect" &&
        res.find("ACK") != res.end() && res["ACK"] == "true")
        return 1;
    else {
        std::cerr << "TCP connect from receiver to sender error" << std::endl;
        return 0;
    }
}

std::map<std::string, std::string> CommandThread::getIpandPortFromSource() {
    std::map<std::string, std::string> step1;
    step1["operation"] = "source";
    step1["storeId"] = sourceServerId;
    step1["keyRange"] = kvStore.getKeyRange(sourceServerId);

    std::string step1_message = jsonParser.MapToJson(step1);

    if (send(migratingStoreSocketSend, step1_message.c_str(), step1_message.size(), 0) < 0) {
        std::cerr << "message sending to data sender error" << std::endl;
        std::map<std::string, std::string> e;
        e["error"] = "0";
        return e;
    }

    std::string buffer = "";
    if (!receiveData(migratingStoreSocketSend, buffer)) {
        std::map<std::string, std::string> e;
        e["error"] = "1";
        return e;
    }
    return jsonParser.JsonToMap(buffer);
}

//todo: implement the sending server, two options, delete and close
int CommandThread::handleSendServer(bool isAdd /*add->delete*/) {
    //todo: need to implement the function after add new server
    if (isAdd) {

    } else {
        std::map<std::string, std::string> closeMap;
        closeMap["operation"] = "close";
        std::string closeString = jsonParser.MapToJson(closeMap);
        if (!send(migratingStoreSocketSend, closeString.c_str(), closeString.size(), 0)) {
            std::cerr << "CommandThread: close server: sending command failed, need manually operation!!!!!!" << std::endl;
        }
    }
//    std::cout << migratingStoreSocketSend << std::endl;

    return 1;
}

int CommandThread::handleReceiveServer() {
    std::string ACKstring = "";
    if (!receiveData(migratingStoreSocketRecv, ACKstring)) {
        std::cerr << "CommandThread: error recieving ACK data from receiver during data transfer";
        return 0;
    }
    std::map<std::string, std::string> ACKmap = jsonParser.JsonToMap(ACKstring);
    if (ACKmap.find("ACK") == ACKmap.end() || ACKmap["ACK"] != "true") {
        std::cerr << "CommandThread: recieving NACK data from receiver during data transfer";
        return 0;
    }

    std::map<std::string, std::string> finishMap;
    finishMap["operation"] = "finish";
    std::string finishResp = jsonParser.MapToJson(finishMap);
    send(migratingStoreSocketRecv, finishResp.c_str(), finishResp.size(), 0);
    finishResp = "";
    receiveData(migratingStoreSocketRecv, finishResp);
    std::cout << "CommandThread: receiver data transfer ACK confirmed" << finishResp << std::endl;
    return 1;
}

int CommandThread::receiveData(int clientSocket, std::string &receivedData) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "CommandThread: receiveData: TCP disconnected.\n";
        } else {
            std::cerr << "CommandThread: receiveData: Error reading from client.\n";
        }
        close(clientSocket);
        return 0;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the received data
    receivedData = std::string(buffer); // Store in the output parameter
    return 1;
}