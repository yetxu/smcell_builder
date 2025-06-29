#include "network_manager.h"
#include "logger.h"
#include <iostream>

NetworkManager& NetworkManager::GetInstance() {
    static NetworkManager instance;
    return instance;
}

bool NetworkManager::Initialize(const char* host, const char* user, 
                              const char* password, const char* database, 
                              unsigned int port) {
    LOG_INFO("Initializing NetworkManager with database connection");
    LOG_INFO("  - Host: %s", host);
    LOG_INFO("  - Port: %u", port);
    LOG_INFO("  - Database: %s", database);
    LOG_INFO("  - User: %s", user);
    
    // 连接到MySQL数据库
    if (!mysql_connector_.Connect(host, user, password, database, port)) {
        LOG_ERROR("Failed to connect to MySQL database: %s", mysql_connector_.GetLastError());
        return false;
    }
    
    LOG_INFO("Successfully connected to MySQL database");
    
    // 加载组网关系表
    if (!LoadNetworkRelations()) {
        LOG_ERROR("Failed to load network relations from database");
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("NetworkManager initialized successfully with %zu network relations", GetRelationCount());
    
    return true;
}

bool NetworkManager::LoadNetworkRelations() {
    LOG_INFO("Loading network relations from database...");
    
    NetworkRelationMap temp_relations;
    
    if (!mysql_connector_.GetNetworkRelations(temp_relations)) {
        LOG_ERROR("Failed to get network relations from database: %s", mysql_connector_.GetLastError());
        return false;
    }
    
    // 线程安全地更新关系表
    {
        std::lock_guard<std::mutex> lock(relations_mutex_);
        network_relations_ = temp_relations;
    }
    
    LOG_INFO("Successfully loaded %zu network relations", temp_relations.size());
    
    // 输出前几个关系作为示例
    size_t count = 0;
    for (const auto& relation : temp_relations) {
        if (count < 5) {  // 只显示前5个
            LOG_INFO("  - IP: %s -> Label: %s", relation.first.c_str(), relation.second.c_str());
        }
        count++;
    }
    
    if (temp_relations.size() > 5) {
        LOG_INFO("  ... and %zu more relations", temp_relations.size() - 5);
    }
    
    return true;
}

std::string NetworkManager::GetLabelByIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(relations_mutex_);
    
    auto it = network_relations_.find(ip);
    if (it != network_relations_.end()) {
        LOG_DEBUG("Found label '%s' for IP '%s'", it->second.c_str(), ip.c_str());
        return it->second;
    } else {
        LOG_DEBUG("No label found for IP '%s', using default", ip.c_str());
        return "";  // 返回空字符串表示未找到
    }
}

std::string NetworkManager::GetLabelByIP(const char* ip) {
    if (!ip) {
        LOG_WARN("GetLabelByIP called with null IP pointer");
        return "";
    }
    return GetLabelByIP(std::string(ip));
}

bool NetworkManager::HasIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(relations_mutex_);
    return network_relations_.find(ip) != network_relations_.end();
}

const NetworkRelationMap& NetworkManager::GetAllRelations() const {
    std::lock_guard<std::mutex> lock(relations_mutex_);
    return network_relations_;
}

bool NetworkManager::ReloadNetworkRelations() {
    LOG_INFO("Reloading network relations from database...");
    return LoadNetworkRelations();
}

size_t NetworkManager::GetRelationCount() const {
    std::lock_guard<std::mutex> lock(relations_mutex_);
    return network_relations_.size();
}

void NetworkManager::Cleanup() {
    if (initialized_) {
        LOG_INFO("Cleaning up NetworkManager");
        mysql_connector_.Disconnect();
        initialized_ = false;
        
        std::lock_guard<std::mutex> lock(relations_mutex_);
        network_relations_.clear();
        
        LOG_INFO("NetworkManager cleanup completed");
    }
} 