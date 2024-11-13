#ifndef KV_STORE_MAP_H
#define KV_STORE_MAP_H

#include <unordered_map>
#include <string>
#include <optional>
#include <shared_mutex> // Use shared_mutex for concurrent reads
#include "JsonParser.h"
/*
Key: store_id
Value:
{
  "ip": "",
  "heartbeatPort":"",
  "addDropPort":"",
  "clientPort":"",
  "alive":"true"/"false",
  ¡°keyNum":""£¬
  ¡°keyRange": ""
}
 */

class KVStoreMap {
private:
    std::unordered_map<std::string, std::string> store;
    mutable std::shared_mutex sharedMutex; // Use shared_mutex for shared and exclusive locks
    JsonParser jsonParser;

    // Internal helper function to update a specific field in the JSON data
    void updateField(const std::string& key, const std::string& field, const std::string& value);

public:
    void write(const std::string& key, const std::string& value);
    std::optional<std::string> read(const std::string& key) const;

    // Getter functions for various fields in the JSON structure
    std::string getIp(const std::string& key) const;
    std::string getHeartbeatPort(const std::string& key) const;
    std::string getAddDropPort(const std::string& key) const;
    std::string getClientPort(const std::string& key) const;
    std::string getStoreStatus(const std::string& key) const;
    std::string getStoreKeyNum(const std::string& key) const;
    std::string getKeyRange(const std::string& key) const;

    // Setter functions to update individual fields in the JSON structure
    void setIp(const std::string& key, const std::string& ip);
    void setHeartbeatPort(const std::string& key, const std::string& port);
    void setAddDropPort(const std::string& key, const std::string& port);
    void setClientPort(const std::string& key, const std::string& port);
    void setStoreStatus(const std::string& key, const std::string& status);
    void setStoreKeyNum(const std::string& key, const std::string& keyNum);
    void setKeyRange(const std::string& key, const std::string& keyRange);

    // Function to set all fields at once
    void setAllFields(const std::string& key, const std::string& ip, const std::string& heartbeatPort,
                      const std::string& addDropPort, const std::string& clientPort, const std::string& status,
                      const std::string& keyNum, const std::string& keyRange);
};

#endif // KV_STORE_MAP_H