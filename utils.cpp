#include"utils.h"
//pthread functions
bool StartThread(pthread_t* thread_id, void* thread_func(void*), void* param){
    int ret = pthread_create(thread_id, NULL, thread_func, param);
    if (ret != 0){
        perror("create thread!\n");
        return false;
    }
    return true;
}

void StopThread(pthread_t thread_id){
    pthread_cancel(thread_id);
}

//time function
void GetTime(char* buffer){
    time_t now;
    struct tm* timenow;
    time(&now);
    timenow = localtime(&now);

    strcpy(buffer, asctime(timenow));
}

//socket function
void BuildSockAddr(const char* ip, const uint16_t port, struct sockaddr_in* sock_addr){
    memset((char*)sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(port);
    sock_addr->sin_addr.s_addr = inet_addr(ip);
}



