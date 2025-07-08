#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>

class DbManager {
public:
    // 单例模式
    static DbManager* GetInstance();
    
    // 初始化数据库连接
    bool Initialize(const char* host, int port, const char* db_name, 
                   const char* user, const char* password);
    
    // 关闭数据库连接
    void Cleanup();
    
    // 读取组网关系表
    bool LoadNetworkRelations(std::map<std::string, std::string>& relations);

private:
    DbManager();
    ~DbManager();
    

    static DbManager* instance_;
    MYSQL* mysql_conn_;
    bool is_connected_;
};

#endif // DB_MANAGER_H

