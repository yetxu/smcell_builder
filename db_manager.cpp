#include "db_manager.h"
#include "logger.h"
#include <cstring>

DbManager* DbManager::instance_ = NULL;

DbManager* DbManager::GetInstance() {
    if (instance_ == NULL) {
        instance_ = new DbManager();
    }
    return instance_;
}

DbManager::DbManager() : mysql_conn_(NULL), is_connected_(false) {
}

DbManager::~DbManager() {
    Cleanup();
}

bool DbManager::Initialize(const char* host, int port, const char* db_name, 
                          const char* user, const char* password) {
    // 如果已经连接，先清理
    if (is_connected_) {
        Cleanup();
    }
    
    // 初始化MySQL连接
    mysql_conn_ = mysql_init(NULL);
    if (mysql_conn_ == NULL) {
        LOG_ERROR("Failed to initialize MySQL connection");
        return false;
    }
    
    // 设置连接选项
    my_bool reconnect = 1;
    mysql_options(mysql_conn_, MYSQL_OPT_RECONNECT, &reconnect);
    
    // 连接到MySQL服务器
    if (mysql_real_connect(mysql_conn_, host, user, password, db_name, port, NULL, 0) == NULL) {
        LOG_ERROR("Failed to connect to MySQL: %s", mysql_error(mysql_conn_));
        mysql_close(mysql_conn_);
        mysql_conn_ = NULL;
        return false;
    }
    
    is_connected_ = true;
    LOG_INFO("Successfully connected to MySQL database: %s", db_name);
    return true;
}

void DbManager::Cleanup() {
    if (mysql_conn_ != NULL) {
        mysql_close(mysql_conn_);
        mysql_conn_ = NULL;
    }
    is_connected_ = false;
}

bool DbManager::LoadNetworkRelations(std::map<std::string,std::string>& relations) {
    if (!is_connected_ || mysql_conn_ == NULL) {
        LOG_ERROR("Not connected to MySQL database");
        return false;
    }
    
    // 清空输出参数
    relations.clear();
    
    // 执行查询
    const char* query = "SELECT vsatIP, toLabel FROM vsatDetails";
    if (mysql_query(mysql_conn_, query) != 0) {
        LOG_ERROR("Failed to execute query: %s", mysql_error(mysql_conn_));
        return false;
    }
    
    // 获取结果集
    MYSQL_RES* result = mysql_store_result(mysql_conn_);
    if (result == NULL) {
        LOG_ERROR("Failed to get result set: %s", mysql_error(mysql_conn_));
        return false;
    }
    
    // 处理结果
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::string vsatIP = row[0];
        std::string toLabel = row[1];
        
        // 将目标ID添加到源ID的列表中
        relations[vsatIP] = toLabel;
    }
    
    // 释放结果集
    mysql_free_result(result);
    
    LOG_INFO("Loaded %zu network relations from database", relations.size());
    return true;
}

