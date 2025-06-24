#ifndef UTILS_H_
#define UTILS_H_
#include<iostream>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fstream>
#include <assert.h>
#include <pthread.h>
#include <string>
#include <sstream>


#include <netpacket/packet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/ip.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ip.h>
#include <netpacket/packet.h>
#include <linux/stddef.h>
#include <sys/select.h>
#include <sys/time.h>

template<typename T>
inline void SafeDelete(T& p){
    if (p){
        delete p;
        p = NULL;
    }
}
//pthread functions
bool StartThread(pthread_t* thread_id, void* thread_func(void*), void* param);
void StopThread(pthread_t thread_id);

//time
void GetTime(char* buffer);

//build socket addr

void BuildSockAddr(const char* ip, const uint16_t port, struct sockaddr_in* sock_addr);

template<typename T>
std::string Type2String(const T& t){
    std::ostringstream os;
    os << t;
    return os.str();
}

template<typename T>
T String2Type(const std::string& str){
    T result;
    std::istringstream is(str);
    is >> result;
    return result;
}
#endif
