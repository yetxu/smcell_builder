#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_
#include"utils.h"
#include"comm_socket.h"
class TcpSocket:public CommonSocket{
public:
    TcpSocket();

    ~TcpSocket();
    
    bool Create();
    bool Create(int socket_fd); //build a socket by sockfd, used in server accept a client

    bool Accept(int* socket_fd, struct sockaddr_in* client_addr);
    bool Listen(int max_num = 10);
    bool GetPeerAddr();

    //send diy-packet, this packet contains a 4-bytes-len packet_header notifying the total length of following meta data,
    //and followed by the meta data of pack
    int SendPacket(char* data, int len);

    //when socket fd occurs a read event, call this function to receive data, return value indicates:
    // ret == -1 : failed to receive
    // ret >= 0, has received ret complete packet(s) and store them in recv_buffer
    // the argument recv_buffer and recved_len can return useful information to the caller, but they are only meaningful when ret >= 1
   
    //when tcp socket has received 1 or more complete packet and stored them in recv_buffer_, function return the number of the complete packets and copy all complete packets to out_recv_buffer, and update the recv_buffer_(the private member of TcpSocket)
    int RecvPacket(char* recv_buffer,  int* recved_len); 

private:
    //a TcpSocket object maintain a RecvBuffer, if it has received a complete packet, notify corresponding module to handle
    enum {
        TCP_BUF_SIZE = 50000
    };
    char recv_buffer_[TCP_BUF_SIZE]; //rev_buffer to store tcp packet
    char* recv_pos_;     //pointer that indicates the beginning of the location where newly received data should store
};
#endif
