#include "network_manager.h"
#include "logger.h"
#include <iostream>

NetworkManager* NetworkManager::instance_ = NULL;
pthread_mutex_t NetworkManager::instance_mutex_ = PTHREAD_MUTEX_INITIALIZER;

NetworkManager::NetworkManager() : 
    initialized_(false),
    db_manager_(NULL) {
    pthread_mutex_init(&relations_mutex_, NULL);
}

NetworkManager::~NetworkManager() {
    Cleanup();
    pthread_mutex_destroy(&relations_mutex_);
}

NetworkManager* NetworkManager::GetInstance() {
    pthread_mutex_lock(&instance_mutex_);
    if (!instance_) {
        instance_ = new NetworkManager();
    }
    pthread_mutex_unlock(&instance_mutex_);
    return instance_;
}

bool NetworkManager::Initialize(const char* host, const char* user, 
                              const char* password, const char* database, 
                              unsigned int port) {
    LOG_INFO("Initializing NetworkManager with database connection");
    LOG_INFO("  - Host: %s", host);
    LOG_INFO("  - Port: %u", port);
    LOG_INFO("  - Database: %s", database);
    LOG_INFO("  - User: %s", user);
    
    pthread_mutex_lock(&relations_mutex_);
    
    if (initialized_) {
        pthread_mutex_unlock(&relations_mutex_);
        return true;
    }

    DbManager* db_manager = DbManager::GetInstance();
    if (!db_manager->Initialize(host, port, database, user, password)) {
        LOG_ERROR("Failed to connect to MySQL database");
        pthread_mutex_unlock(&relations_mutex_);
        return false;
    }
    
    LOG_INFO("Successfully connected to MySQL database");
    
    if (!LoadNetworkRelations()) {
        LOG_ERROR("Failed to load network relations from database");
        db_manager->Cleanup();
        pthread_mutex_unlock(&relations_mutex_);
        return false;
    }
    
    initialized_ = true;
    size_t count = network_relations_.size();
    pthread_mutex_unlock(&relations_mutex_);
    
    LOG_INFO("NetworkManager initialized successfully with %zu network relations", count);
    return true;
}

bool NetworkManager::LoadNetworkRelations() {
    LOG_INFO("Loading network relations from database...");
    
    NetworkRelationMap temp_relations;
    DbManager* db_manager = DbManager::GetInstance();
    
    if (!db_manager->LoadNetworkRelations(temp_relations)) {
        LOG_ERROR("Failed to get network relations from database");
        return false;
    }
    
    pthread_mutex_lock(&relations_mutex_);
    network_relations_ = temp_relations;
    size_t relation_count = network_relations_.size();
    pthread_mutex_unlock(&relations_mutex_);
    
    LOG_INFO("Successfully loaded %zu network relations", relation_count);
    
    // 输出前几个关系作为示例
    size_t count = 0;
    pthread_mutex_lock(&relations_mutex_);
    for (NetworkRelationMap::const_iterator it = network_relations_.begin(); 
         it != network_relations_.end() && count < 5; ++it) {
        LOG_INFO("  - IP: %s -> Label: %s", it->first.c_str(), it->second.c_str());
        count++;
    }
    pthread_mutex_unlock(&relations_mutex_);
    
    if (relation_count > 5) {
        LOG_INFO("  ... and %zu more relations", relation_count - 5);
    }
    
    return true;
}

std::string NetworkManager::GetLabelByIP(const std::string& ip) {
    pthread_mutex_lock(&relations_mutex_);
    NetworkRelationMap::const_iterator it = network_relations_.find(ip);
    std::string label = (it != network_relations_.end()) ? it->second : "";
    pthread_mutex_unlock(&relations_mutex_);
    
    if (!label.empty()) {
        LOG_DEBUG("Found label '%s' for IP '%s'", label.c_str(), ip.c_str());
    } else {
        LOG_DEBUG("No label found for IP '%s', using default", ip.c_str());
    }
    
    return label;
}

std::string NetworkManager::GetLabelByIP(const char* ip) {
    if (!ip) {
        LOG_WARN("GetLabelByIP called with null IP pointer");
        return "";
    }
    return GetLabelByIP(std::string(ip));
}

bool NetworkManager::HasIP(const std::string& ip) {
    pthread_mutex_lock(&relations_mutex_);
    bool exists = (network_relations_.find(ip) != network_relations_.end());
    pthread_mutex_unlock(&relations_mutex_);
    return exists;
}

const NetworkRelationMap& NetworkManager::GetAllRelations() const {
    // 注意：调用者需要自行确保线程安全
    return network_relations_;
}

bool NetworkManager::ReloadNetworkRelations() {
    LOG_INFO("Reloading network relations from database...");
    return LoadNetworkRelations();
}

size_t NetworkManager::GetRelationCount() const {
    pthread_mutex_lock(&relations_mutex_);
    size_t count = network_relations_.size();
    pthread_mutex_unlock(&relations_mutex_);
    return count;
}

void NetworkManager::Cleanup() {
    pthread_mutex_lock(&relations_mutex_);
    
    if (initialized_) {
        LOG_INFO("Cleaning up NetworkManager");
        DbManager* db_manager = DbManager::GetInstance();
        db_manager->Cleanup();
        network_relations_.clear();
        initialized_ = false;
        LOG_INFO("NetworkManager cleanup completed");
    }
    
    pthread_mutex_unlock(&relations_mutex_);
}
