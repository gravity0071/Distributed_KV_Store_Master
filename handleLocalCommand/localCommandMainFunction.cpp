//
// Created by Shawn Wan on 2024/11/6.
//
#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <memory>
#include <map>
#include <string>
#include <future>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>
#include <thread>

void CommandThread::testAddServerIntoKvStore() {
    kvStore.setAllFields("store4",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "8084",            // Add/Drop Port
                         "8083",            // Client Port
                         "true",            // Status
                         "3000",             // KeyNum
                         "0-3000",           // KeyRange
                         "store5",          // Left Store ID
                         "store6"
    );
    kvStore.setAllFields("store5",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "8086",            // Add/Drop Port
                         "8085",            // Client Port
                         "true",            // Status
                         "5000",             // KeyNum
                         "3000-8000",         // KeyRange
                         "store6",          // Left Store ID
                         "store4"           // Right Store ID
    );
    kvStore.setAllFields("store6",          // Key
                         "127.0.0.1",     // IP
                         "",            // Heartbeat Port
                         "9094",            // Add/Drop Port
                         "9095",            // Client Port
                         "true",            // Status
                         "2001",             // KeyNum
                         "8000-10001",         // KeyRange
                         "store4",          // Left Store ID
                         "store5"           // Right Store ID
    );
    consistentMap.addNew("0-10001", "0-3000", "store1");
    consistentMap.addNew("0-10001", "8000-10001", "store3");
    consistentMap.addNew("0-8000", "3000-8000", "store2");
//    sharedVector.add("store2");
}

void CommandThread::operator()() {
//    testAddServerIntoKvStore();
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
    if (!startNewServerAndSetNewPortsintoTmp(false)) {
        std::cerr << "CommandThreadL starting a new kv-server failed, please try again" << std::endl;
        closeAllConnection();
        kvStore.deleteData(newServerId);
        return 0;
    }
    sourceServerId = checkWhichServerMoveFrom();
    if (sourceServerId == "-1") {
        std::cout << "there has no server in the system" << std::endl;
    }
    destServerId = newServerId;

    sharedVector.add(sourceServerId);
    sharedVector.add(destServerId);

    if (!buildConnect(sourceServerId, destServerId)) {
        kvStore.deleteData(destServerId);
        closeAllConnection();
        std::cerr << "build connection to two servers failed" << std::endl;
        return 0;
    }

    size_t startKey = -1;
    size_t endKey = -1;
    consistentMap.parseRange(kvStore.getKeyRange(sourceServerId), startKey, endKey);
    size_t middle = (startKey + ((endKey + HASH_KEY_RANGE - startKey - 2) % HASH_KEY_RANGE) / 2) % HASH_KEY_RANGE +
                    1;

    movingKeyRange = std::to_string(startKey) + "-" + std::to_string(middle);

    if (!receiveAndSendDataBetweenServers(true)) {
        kvStore.deleteData(destServerId);
        closeAllConnection();
        std::cerr << "communicating between servers error" << std::endl;
        return 0;
    }
    closeAllConnection();
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
    sharedVector.add(destServerId);
    sharedVector.add(sourceServerId);
    if (destServerId == "-1") {
        closeAllConnection();
        std::cerr << "assign destServerId error" << std::endl;
        return 0;
    }

    if (!buildConnect(sourceServerId, destServerId)) {
        closeAllConnection();
        std::cerr << "build connection to two servers failed" << std::endl;
        return 0;
    }
    movingKeyRange = kvStore.getKeyRange(sourceServerId);
    if (!receiveAndSendDataBetweenServers(false)) {
        closeAllConnection();
        std::cerr << "communicating between servers error" << std::endl;
        return 0;
    }
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
            std::cerr << "CommandThread: adjacent servers are migrating" << std::endl;
            return "-1";
        }
        return leftAdjacent;
    }
    bool rightIsMigrating = sharedVector.have(rightAdjacent);
    bool leftIsMigrating = sharedVector.have(leftAdjacent);
    if (rightIsMigrating && leftIsMigrating) {
        std::cerr << "CommandThread: adjacent servers are migrating" << std::endl;
        return "-1";
    } else if (rightIsMigrating)
        return leftAdjacent;
    else if (leftIsMigrating)
        return rightAdjacent;
    else {
        int leftServerKeyNum = std::stoi(kvStore.getStoreKeyNum(leftAdjacent));
        int rightServerKeyNum = std::stoi(kvStore.getStoreKeyNum(rightAdjacent));
        return leftServerKeyNum > rightServerKeyNum ?  rightAdjacent : leftAdjacent;
    }
}

std::string CommandThread::checkWhichServerMoveFrom() {
    if (kvStore.size() == 0) {
        return "-1"; // Return an empty string if the map is empty
    }
    return kvStore.getKeyWithMaxKeyNum();
}

void CommandThread::closeAllConnection() {
    close(migratingStoreSocketSend);
    close(migratingStoreSocketRecv);
    migratingStoreSocketSend = -1;
    migratingStoreSocketRecv = -1;
    sharedVector.remove(sourceServerId);
    sharedVector.remove(destServerId);
    sourceServerId = "-1";
    destServerId = "-1";
    newServerClientPort = -1;
    newServerCommandPort = -1;
    newServerId = "";
    movingKeyRange = "";
}

int CommandThread::receiveAndSendDataBetweenServers(bool isAdd) {
    if (!sendSourIpPortToDes()) {
        std::cerr << "CommandThread: failed on the building connection between destination and source server"
                  << std::endl;
        return 0;
    }

    std::future<int> resultSender = std::async(std::launch::async, &CommandThread::handleSendServer, this, isAdd);
    std::future<int> resultReceiver = std::async(std::launch::async, &CommandThread::handleReceiveServer, this);

    int sendResult = resultSender.get();
    int recvResult = resultReceiver.get();

    if (sendResult == 1 && recvResult == 1) {
        operationAfterRemovingOrAdding(isAdd);
    }

    return (sendResult == 0 && recvResult == 0) ? 0 : 1;
}

int CommandThread::sendSourIpPortToDes() {
    //get the sender ip and port
    std::map<std::string, std::string> SourceipAddressPort = getIpandPortFromSource();
    if (SourceipAddressPort.find("error") != SourceipAddressPort.end()) {
        std::cerr << "CommandThread: getIpandPortFromSource error" << std::endl;
        return 0;
    }
    std::string sourceIp = "";
    int sourcePort = 0;
    if (SourceipAddressPort.find("sender_Ip") != SourceipAddressPort.end() &&
        SourceipAddressPort.find("sender_Port") != SourceipAddressPort.end()) {
        sourceIp = SourceipAddressPort["sender_Ip"];
        sourcePort = std::stoi(SourceipAddressPort["sender_Port"]);
    } else {
        std::cerr << "CommandThread: source Ip and port get error" << std::endl;
        return 0;
    }

    //send the ip address to the recv server
    if (!sendIpPorttoRecv(sourceIp, sourcePort)) {
        std::cerr << "CommandThread: sendIpPrttoRecv error" << std::endl;
        return 0;
    }

    return 1;
}

int CommandThread::sendIpPorttoRecv(std::string &sourceIp, int sourcePort) {
    std::map<std::string, std::string> step3;
    step3["operation"] = "recv";
    step3["storeId"] = destServerId;
    step3["keyRange"] = movingKeyRange;
    step3["sourceIp"] = kvStore.getIp(sourceServerId);
    step3["sourcePort"] = std::to_string(sourcePort);

    std::string step3_message = jsonParser.MapToJson(step3);

    if (send(migratingStoreSocketRecv, step3_message.c_str(), step3_message.size(), 0) < 0) {
        std::cerr << "CommandThread: message sending to data receiver error (step 3)" << std::endl;
        return 0;
    }


    std::string buffer = "";
    if (!receiveData(migratingStoreSocketRecv, buffer)) {
        std::cerr << "CommandThread: message sending to data receiver error (step 4)" << std::endl;
        return 0;
    }

    std::map<std::string, std::string> res = jsonParser.JsonToMap(buffer);
    if (res.find("operation") != res.end() && res["operation"] == "connect" &&
        res.find("ACK") != res.end() && res["ACK"] == "true") {
        return 1;
    } else {
        std::cerr << "CommandThread: TCP connect from receiver to sender error" << std::endl;
        return 0;
    }
}

std::map<std::string, std::string> CommandThread::getIpandPortFromSource() {
    std::map<std::string, std::string> step1;
    step1["operation"] = "source";
    step1["storeId"] = sourceServerId;
    step1["keyRange"] = movingKeyRange;

    std::string step1_message = jsonParser.MapToJson(step1);

    if (send(migratingStoreSocketSend, step1_message.c_str(), step1_message.size(), 0) < 0) {
        std::cerr << "CommandThread: message sending to data sender error (step 1)" << std::endl;
        std::map<std::string, std::string> e;
        e["error"] = "0";
        return e;
    }

    std::string buffer = "";
    if (!receiveData(migratingStoreSocketSend, buffer)) {
        std::cerr << "CommandThread: message sending to data receiver error (step 2)" << std::endl;
        std::map<std::string, std::string> e;
        e["error"] = "1";
        return e;
    }
    return jsonParser.JsonToMap(buffer);
}

int CommandThread::handleSendServer(bool isAdd /*add->delete*/) {
    std::map<std::string, std::string> isFinishedMap;
    isFinishedMap["operation"] = "finished";
    std::string isFinished = jsonParser.MapToJson(isFinishedMap);
    if (!send(migratingStoreSocketSend, isFinished.c_str(), isFinished.size(), 0)) {
        std::cerr << "Master: CommandThread: isFinished message sending error." << std::endl;
        return 0;
    }
    std::string ACKstring = "";
    if (!receiveData(migratingStoreSocketSend, ACKstring)) {
        std::cerr << "CommandThread: error receiving ACK data from receiver during data transfer (step 6)";
        return 0;
    }
    std::map<std::string, std::string> ACKmap = jsonParser.JsonToMap(ACKstring);
    if (ACKmap.find("ACK") == ACKmap.end() || ACKmap["ACK"] != "true") {
        std::cerr << "CommandThread: recieving NACK data from receiver during data transfer (step 6)";
        return 0;
    }

    if (isAdd) {
        std::map<std::string, std::string> deleteMap;
        deleteMap["operation"] = "delete";
        deleteMap["keyRange"] = movingKeyRange;
        std::string deleteString = jsonParser.MapToJson(deleteMap);
        if (!send(migratingStoreSocketSend, deleteString.c_str(), deleteString.size(), 0)) {
            std::cerr << "CommandThread: delete key server: sending command failed, need manually operation!!!!!!"
                      << std::endl;
        }
    } else {
        std::map<std::string, std::string> closeMap;
        closeMap["operation"] = "close";
        std::string closeString = jsonParser.MapToJson(closeMap);
        if (!send(migratingStoreSocketSend, closeString.c_str(), closeString.size(), 0)) {
            std::cerr << "CommandThread: close server: sending command failed, need manually operation!!!!!!"
                      << std::endl;
        }
    }

    return 1;
}

int CommandThread::handleReceiveServer() {
    std::map<std::string, std::string> isFinishedMap;
    isFinishedMap["operation"] = "finished";
    std::string isFinished = jsonParser.MapToJson(isFinishedMap);
    if (!send(migratingStoreSocketRecv, isFinished.c_str(), isFinished.size(), 0)) {
        std::cerr << "Master: CommandThread: isFinished message sending error." << std::endl;
        return 0;
    }

    std::string ACKstring = "";
    if (!receiveData(migratingStoreSocketRecv, ACKstring)) {
        std::cerr << "CommandThread: error receiving ACK data from receiver during data transfer (step 6)";
        return 0;
    }
    std::map<std::string, std::string> ACKmap = jsonParser.JsonToMap(ACKstring);
    if (ACKmap.find("ACK") == ACKmap.end() || ACKmap["ACK"] != "true") {
        std::cerr << "CommandThread: recieving NACK data from receiver during data transfer (step 6)";
        return 0;
    }

    std::map<std::string, std::string> finishMap;
    finishMap["operation"] = "finish";
    std::string finishResp = jsonParser.MapToJson(finishMap);
    send(migratingStoreSocketRecv, finishResp.c_str(), finishResp.size(), 0);
    finishResp = "";
    if(!receiveData(migratingStoreSocketRecv, finishResp)){
        std::cerr << "fuck it" << std::endl;
    }
    return 1;
}

int CommandThread::receiveData(int clientSocket, std::string &receivedData) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Master: CommandThread: receiveData: TCP disconnected.\n";
        } else {
            std::cerr << "Master: CommandThread: receiveData: Error reading from client.\n";
        }
        close(clientSocket);
        return 0;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the received data
    receivedData = std::string(buffer); // Store in the output parameter
    return 1;
}

void CommandThread::operationAfterRemovingOrAdding(bool isAdd) {
    if (isAdd) {
        std::string oldRange = kvStore.getKeyRange(sourceServerId);
        consistentMap.addNew(oldRange, movingKeyRange, destServerId);

        size_t newStart, newEnd, oldStart, oldEnd;
        consistentMap.parseRange(movingKeyRange, newStart, newEnd);
        consistentMap.parseRange(oldRange, oldStart, oldEnd);
        std::string newRange = std::to_string(newEnd) + "-" + std::to_string(oldEnd);

        consistentMap.removeRange(oldRange, newRange);

        kvStore.setStoreStatus(destServerId, "true");

        kvStore.setLeftStoreId(destServerId, kvStore.getLeftStoreId(sourceServerId));
        kvStore.setRightStoreId(kvStore.getLeftStoreId(sourceServerId), destServerId);
        kvStore.setRightStoreId(destServerId, sourceServerId);
        kvStore.setLeftStoreId(sourceServerId, destServerId);

        size_t keyNum = (newEnd + HASH_KEY_RANGE - newStart - 2) % HASH_KEY_RANGE + 2;
        kvStore.setStoreKeyNum(destServerId, std::to_string(keyNum));

        int sourceKeyNum = std::stoi(kvStore.getStoreKeyNum(sourceServerId));
        kvStore.setStoreKeyNum(sourceServerId, std::to_string(sourceKeyNum - static_cast<int>(keyNum)));
        kvStore.setKeyRange(sourceServerId, std::to_string(newEnd) + "-" + std::to_string(oldEnd));
        kvStore.setKeyRange(destServerId, std::to_string(newStart) + "-" + std::to_string(newEnd));
    } else {
        consistentMap.removeRange(kvStore.getKeyRange(sourceServerId), kvStore.getKeyRange(destServerId));

        kvStore.setLeftStoreId(kvStore.getRightStoreId(sourceServerId), kvStore.getLeftStoreId(sourceServerId));
        kvStore.setRightStoreId(kvStore.getLeftStoreId(sourceServerId), kvStore.getRightStoreId(sourceServerId));
        kvStore.setStoreKeyNum(destServerId, std::to_string(
                std::stoi(kvStore.getStoreKeyNum(destServerId)) + std::stoi(kvStore.getStoreKeyNum(sourceServerId))));

        std::string newKeyRange = "";
        for (auto now_server: consistentMap) {
            if (now_server.second == destServerId) {
                newKeyRange = now_server.first;
                break;
            }
        }
        kvStore.setKeyRange(destServerId, newKeyRange);
        kvStore.deleteData(sourceServerId);
    }

//    kvStore.displayAllData();
//    consistentMap.displayHashRing();
}


std::string generateRandomServerId(size_t length = 6) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string serverId;
    for (size_t i = 0; i < length; ++i) {
        serverId += characters[distribution(generator)];
    }

    return serverId;
}

int CommandThread::startNewServerAndSetNewPortsintoTmp(bool isTheFirstServer) {
    Server tmp(0);

    if (!tmp.initialize()) {
        std::cerr << "CommandThread: Temporary server initialization failed" << "." << std::endl;
        tmp.closeServer(); // Cleanup: Ensure the server is closed
        return 0;
    }
    std::string tmp_port = std::to_string(tmp.getAssignedPort());

    newServerId = generateRandomServerId();
    std::string initiateServerCommand = std::string(INITATE_COMMAND_ADDRESS) + " " +
                                        newServerId + " " +
                                        std::string(MASTER_IP) + " " +
                                        std::string(HEARTBEAT_PORT) + " " + tmp_port;

    std::thread([](std::string command) {
        std::cout << "Executing command in background: " << command << std::endl;

        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "CommandThread: Failed to execute command: " << command << std::endl;
        } else {
            char buffer[128];
//            std::cout << "Command Output:" << std::endl;

            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
//                std::cout << buffer;
            }

            int exitStatus = pclose(pipe);
            if (exitStatus != 0) {
                std::cerr << "CommandThread: Command exited with status: " << exitStatus << std::endl;
            }
        }
    }, initiateServerCommand).detach();

    int tmpSocket = tmp.acceptConnection();
    if (tmpSocket < 0) {
        std::cerr << "CommandThread: Accepting connection from the new server timed out." << std::endl;
        tmp.closeServer(); // Cleanup
        return 0;
    }

    std::string buffer = "";

    if (!receiveData(tmpSocket, buffer)) {
        std::cerr << "CommandThread: Receiving ports from the new server failed. (initiate a new server)\n";
        tmp.closeConnection(tmpSocket); // Cleanup
        tmp.closeServer();             // Cleanup
        return 0;
    }

    std::map<std::string, std::string> portMap = jsonParser.JsonToMap(buffer);
    if (portMap.find("storeId") == portMap.end() || portMap["storeId"] != newServerId) {
        std::cerr << "CommandThread: Wrong command sent to master from server " << newServerId << std::endl;
        tmp.closeConnection(tmpSocket); // Cleanup
        tmp.closeServer();             // Cleanup
        return 0;
    }

    if (portMap.find("clientPort") != portMap.end()) {
        newServerClientPort = std::stoi(portMap["clientPort"]);
    } else {
        std::cerr << "CommandThread: Client port read failed.\n";
        tmp.closeConnection(tmpSocket); // Cleanup
        tmp.closeServer();             // Cleanup
        return 0;
    }

    if (portMap.find("commandPort") != portMap.end()) {
        newServerCommandPort = std::stoi(portMap["commandPort"]);
    } else {
        std::cerr << "CommandThread: Command port read failed.\n";
        tmp.closeConnection(tmpSocket); // Cleanup
        tmp.closeServer();             // Cleanup
        return 0;
    }

    tmp.closeConnection(tmpSocket); // Cleanup
    tmp.closeServer();             // Cleanup

    std::string keyNum = isTheFirstServer ? std::to_string(HASH_KEY_RANGE) : "";
    std::string keyRange = isTheFirstServer ? "0-" + std::to_string(HASH_KEY_RANGE) : "";
    std::string leftStoreId = isTheFirstServer ? newServerId : "";
    std::string rightStoreId = isTheFirstServer ? newServerId : "";
    std::string status = isTheFirstServer ? "true" : "false";

    kvStore.setAllFields(newServerId, KV_STORE_SERVER_IP, "", std::to_string(newServerCommandPort),
                         std::to_string(newServerClientPort), status, keyNum, keyRange, leftStoreId, rightStoreId);
    if (isTheFirstServer)
        consistentMap.addNew("0-0", "0-10000", newServerId);
    return 1;
}