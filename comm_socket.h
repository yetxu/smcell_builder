#ifndef COMMON_SOCKET_H_
#define COMMON_SOCKET_H_
#include"utils.h"
class CommonSocket{
public:
    CommonSocket();
    virtual ~CommonSocket();

    virtual bool Create(){return true;};
    bool Bind(const char* listen_ip, uint16_t listen_port);
    bool Connect(const char* peer_ip, uint16_t peer_port);
    bool SetBlocking(bool);
    bool SetSendBufSize(int);
    bool SetRecvBufSize(int);

    void Close();

    int Send(char* buf, int len);
    int Recv(char* buf, int buf_len);

    const struct sockaddr_in& local_addr(){
        return local_addr_;
    }

    const struct sockaddr_in& peer_addr(){
        return peer_addr_;
    }
    
    const int& socket_fd()const{
        return socket_fd_;
    }
    const int& if_status()const{
        return if_status_;
    }
    void set_if_status(int status){
        if_status_ = status;
    }

protected:
    int socket_fd_;
    int if_status_;
    struct sockaddr_in local_addr_;
    struct sockaddr_in peer_addr_;
};
#endif
