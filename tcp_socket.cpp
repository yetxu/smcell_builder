#include"tcp_socket.h"
#include"comm_socket.h"
TcpSocket::TcpSocket(){
    recv_pos_ = recv_buffer_;
}

TcpSocket::~TcpSocket(){

}


bool TcpSocket::Create(){
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0){
        perror("build tcp socket failed!\n");
        return false;
    }
    return true;
}

bool TcpSocket::Create(int sock_fd){
    socket_fd_ = sock_fd;
    if (socket_fd_ < 0){
        perror("build tcp socket failed!\n");
        return false;
    }
    return true;
}
bool TcpSocket::Accept(int* socket_fd, struct sockaddr_in* client_addr){
    int socket_len = sizeof(struct sockaddr_in);
    int ret  = accept(socket_fd_, (sockaddr*)client_addr, (socklen_t*)&socket_len);
    if (ret < 0){
        perror("tcp accept failed!\n");
        return false;
    }
    *socket_fd = ret;
    return true;
}

bool TcpSocket::GetPeerAddr(){
    socklen_t len = sizeof(peer_addr_);
    if (getpeername(socket_fd_, (sockaddr*)&peer_addr_, &len) < 0){
        perror("tcp socket get peer addr failed!\n");
        return false;
    }
    return true;
}

bool TcpSocket::Listen(int max_num){
    int ret = listen(socket_fd_, max_num);
    if (ret < 0){
        perror("tcp listen failed!");
        return false;
    }
    return true;
}

int TcpSocket::SendPacket(char* data, int len){
    return 0;
}

int TcpSocket::RecvPacket(char* out_recv_buffer, int* recved_len){

    assert(recv_pos_ - recv_buffer_ < TCP_BUF_SIZE);

    int recv_bytes = Recv(recv_pos_, TCP_BUF_SIZE + recv_buffer_ - recv_pos_);

    if (recv_bytes == 0){//peer side close the connection
        perror("peer disconnect\n");
        return -1;
    }
    if (recv_bytes < 0){ //error happens
        perror("tcp receive failed\n");
        return -1;
    }
    
    //received data
    recv_pos_ += recv_bytes;

    int complete_packet_num = 0, complete_packet_bytes = 0, cur_packet_len = 0;
    char* read_pos = recv_buffer_;

    const int kHeadLen = 4;


    while(true){
        if (recv_pos_ - read_pos < kHeadLen){
            break;
        }
        cur_packet_len = *(int*)read_pos;   //get data length of another packet
        read_pos += kHeadLen;
        if(read_pos + cur_packet_len > recv_pos_ ){//not another complete pack
            break;
        }
        read_pos += cur_packet_len;
        complete_packet_bytes += (kHeadLen + cur_packet_len);//another complete packet
        complete_packet_num ++;
        
    }

    if (complete_packet_bytes){
        //copy data to out buffer
        memcpy(out_recv_buffer, recv_buffer_, complete_packet_bytes);
        *recved_len = complete_packet_bytes;

        //update the recv_buffer_
        int left_size = recv_pos_ - recv_buffer_ - complete_packet_bytes;
        memcpy(recv_buffer_, read_pos, left_size);
        recv_pos_ = recv_buffer_ + left_size;
    }

    return complete_packet_num;
}
