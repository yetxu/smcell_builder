#ifndef NETWORK_MANAGER_H_
#define NETWORK_MANAGER_H_

#include <map>
#include <string>
#include <mutex>
#include "db_manager.h"
#include "cell_factory.h"

class NetworkManager {
public:
    static NetworkManager& GetInstance();
    
    // 初始化网络管理器
    bool Initialize(const char* host, const char* user, 
                   const char* password, const char* database, 
                   unsigned int port = 3306);
    
    // 加载组网关系表
    bool LoadNetworkRelations();
    
    // 根据IP获取标签
    std::string GetLabelByIP(const std::string& ip);
    
    // 根据IP获取标签（重载版本，支持C风格字符串）
    std::string GetLabelByIP(const char* ip);
    
    // 检查IP是否存在
    bool HasIP(const std::string& ip);
    
    // 获取所有组网关系
    const NetworkRelationMap& GetAllRelations() const;
    
    // 重新加载组网关系表
    bool ReloadNetworkRelations();
    
    // 获取关系表大小
    size_t GetRelationCount() const;
    
    // 清理资源
    void Cleanup();
    
    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

private:
    NetworkManager() : initialized_(false) {}
    ~NetworkManager() { Cleanup(); }
    
    // 禁用拷贝构造和赋值
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

private:
    MySQLConnector mysql_connector_;
    NetworkRelationMap network_relations_;
    mutable std::mutex relations_mutex_;
    bool initialized_;
};

// 全局访问宏
#define g_network_manager NetworkManager::GetInstance()

#endif // NETWORK_MANAGER_H_ 