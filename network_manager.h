#ifndef NETWORK_MANAGER_H_
#define NETWORK_MANAGER_H_

#include <map>
#include <string>
#include "db_manager.h"
#include "cell_factory.h"
#include <pthread.h>

typedef std::map<std::string, std::string> NetworkRelationMap;

class NetworkManager {
    mutable pthread_mutex_t relations_mutex_;  // 添加mutable
    bool initialized_;
    NetworkRelationMap network_relations_;
    DbManager* db_manager_;

    static NetworkManager* instance_;
    static pthread_mutex_t instance_mutex_;

    NetworkManager(const NetworkManager&);
    NetworkManager& operator=(const NetworkManager&);

public:
    static NetworkManager* GetInstance();
    
    bool Initialize(const char* host, const char* user, 
                   const char* password, const char* database, 
                   unsigned int port = 3306);
    
    bool LoadNetworkRelations();
    bool LoadNetworkRelations_NoLock();
    std::string GetLabelByIP(const std::string& ip);
    std::string GetLabelByIP(const char* ip);
    bool HasIP(const std::string& ip);
    const NetworkRelationMap& GetAllRelations() const;
    bool ReloadNetworkRelations();
    size_t GetRelationCount() const;
    void Cleanup();
    bool IsInitialized() const { return initialized_; }

private:
    NetworkManager();
    ~NetworkManager();
};


#endif // NETWORK_MANAGER_H_
