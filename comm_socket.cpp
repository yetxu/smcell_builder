#include"comm_socket.h"
CommonSocket::CommonSocket(){
    memset(&local_addr_, 0,  sizeof(local_addr_));
    memset(&peer_addr_, 0, sizeof(peer_addr_));
}
CommonSocket::~CommonSocket(){
    Close();
}
bool CommonSocket::Bind(const char* listen_ip, uint16_t listen_port){
       local_addr_.sin_family = AF_INET;
       local_addr_.sin_port = htons(listen_port);
       local_addr_.sin_addr.s_addr = inet_addr(listen_ip);
       uint32_t option = 1;
       if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option)) < 0){
           perror("set socket reuse addr\n");
           return false;
       }
       if (bind(socket_fd_, (sockaddr*)&local_addr_, sizeof(local_addr_)) != 0){
           perror("bind socket\n");
           return false;
       }
       return true;
}
bool CommonSocket::SetSendBufSize(int size){
    int result = setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int));
    if (result < 0){
        perror("setsockopt send buf size\n");
        return false;
    }
    return true;
}

bool CommonSocket::SetRecvBufSize(int size){
    int result = setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(int));
    if (result < 0){
        perror("setsockopt recv buf size\n");
        return false;
    }
    return true;
}


bool CommonSocket::SetBlocking(bool block){
    int opts;
    opts = fcntl(socket_fd_, F_GETFL);
    if (opts < 0){
        perror("fcntl get");
        return false;
    }

    if (block){ //set block mode
        opts = opts & (~O_NONBLOCK);
    }else{     // set nonblock mode
        opts = opts | O_NONBLOCK;
    }

    if (fcntl(socket_fd_, F_SETFL, opts) < 0){
        perror("fcntl set");
        return false;
    }
    return true;
}

bool CommonSocket::Connect(const char* peer_ip, uint16_t peer_port){
    peer_addr_.sin_family = AF_INET;
    peer_addr_.sin_addr.s_addr = inet_addr(peer_ip);
    peer_addr_.sin_port = htons(peer_port);
    int result = connect(socket_fd_, (sockaddr*)&peer_addr_, sizeof(peer_addr_));
    if (result < 0){
        perror("Common connect\n");
    }
    return result  == 0;
}   

void CommonSocket::Close(){
    close(socket_fd_);
}

int CommonSocket::Send(char*buf, int len){
    int result = send(socket_fd_, buf, len, 0);
    if (result < 0){
        perror("Common send to connected peer\n");
    }
    return result;
}
int CommonSocket::Recv(char* buf, int buf_len){
    int recv_bytes = recv(socket_fd_, buf, buf_len, 0);
    if (recv_bytes < 0){
        perror("Common recv from connected peer \n");
    }
    return recv_bytes;
}

