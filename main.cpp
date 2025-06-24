#include<iostream>
#include"utils.h"
#include"logger.h"
#include"publisher.h"
#include"parameter.h"
#define LOG_FILE "pub_video.log"
using namespace std;

Logger g_logger;
Parameter gParameter;
int main(int argc, char* argv[]){

    g_logger.Init(LOG_FILE);
    gParameter.LoadConfig();
    Publisher publisher;
    if (! publisher.Init(&gParameter)){
        cout << "Publisher init failed" << endl;
        return -1;
    }

    publisher.StartWork();
    pthread_join(publisher.recv_thread(), NULL);
    pthread_join(publisher.inner_com_thread(), NULL);
    return 0;
}
