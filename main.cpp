#include<iostream>
#include"utils.h"
#include"logger.h"
#include"publisher.h"
#include"parameter.h"
#include"network_manager.h"
#define LOG_FILE "pub_video.log"
using namespace std;

Parameter gParameter;
int main(int argc, char* argv[]){

    // 初始化日志系统，设置日志级别为INFO
    g_logger.Init(LOG_FILE, LOG_LEVEL_INFO);
    LOG_INFO("SMCellBuilder starting up...");
    
    gParameter.LoadConfig();
    LOG_INFO("Configuration loaded successfully");
    
    // 初始化网络管理器并加载组网关系表
    LOG_INFO("Initializing network manager...");
    if (!g_network_manager.Initialize(gParameter.db_host, gParameter.db_user, 
                                    gParameter.db_pass, gParameter.db_name, 
                                    gParameter.db_port)) {
        LOG_ERROR("Failed to initialize network manager");
        cout << "Network manager initialization failed" << endl;
        return -1;
    }
    LOG_INFO("Network manager initialized successfully");
    
    Publisher publisher;
    if (! publisher.Init(&gParameter)){
        LOG_ERROR("Publisher initialization failed");
        cout << "Publisher init failed" << endl;
        return -1;
    }
    LOG_INFO("Publisher initialized successfully");

    publisher.StartWork();
    LOG_INFO("Publisher started, waiting for threads to complete");
    
    pthread_join(publisher.recv_thread(), NULL);
    pthread_join(publisher.inner_com_thread(), NULL);
    
    LOG_INFO("SMCellBuilder shutting down");
    return 0;
}
