#include"udp_socket.h"
#include"comm_socket.h"
UdpSocket::UdpSocket(){
}

UdpSocket::~UdpSocket(){
}
bool UdpSocket::Create(){
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0){
        perror("build udp socket failed!\n");
        return false;
    }
    return true;
}

int UdpSocket::SendTo(char* buf, int len, const char* dst_ip, uint16_t dst_port){
    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(dst_port);
    sock_addr.sin_addr.s_addr = inet_addr(dst_ip);
    int result = sendto(socket_fd_, buf, len, 0, (sockaddr*)&sock_addr, sizeof(sockaddr));
    if (result < 0){
        perror("udp send\n");
    }
    return result;
}
int UdpSocket::SendTo(char* buf, int len, const struct sockaddr_in& sock_addr){
     int result = sendto(socket_fd_, buf, len, 0, (sockaddr*)&sock_addr, sizeof(sockaddr));
    if (result < 0){
        perror("udp send\n");
    }
    return result;
}

int UdpSocket::RecvFrom(char* buf, int buf_len, uint32_t* from_ip, uint16_t* from_port){
    struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(sock_addr);
    int recv_bytes = recvfrom(socket_fd_, buf, buf_len, 0, (sockaddr*)&sock_addr, &addr_len);
    if (recv_bytes < 0){
        perror("udp recv\n");
    }
    if (from_ip){
        *from_ip = ntohl(sock_addr.sin_addr.s_addr);
    }
    if (from_port){
        *from_port = ntohs(sock_addr.sin_port);
    }

    return recv_bytes;
}

int UdpSocket::RecvFrom(char* buf, int buf_len, struct sockaddr_in* sock_addr){
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int recv_bytes = recvfrom(socket_fd_, buf, buf_len, 0, (sockaddr*)sock_addr, &addr_len);
    if (recv_bytes < 0){
        perror("udp recv\n");
    }
    return recv_bytes;
}


int UdpSocket::WaitMsg(int time_out, int *elapsed_time){
    const int MSG_IDLE = 0, MSG_ARRIVAL = 1;
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(socket_fd_, &read_fds);
	struct timeval time_val;
	time_val.tv_sec = time_out / 1000;
	time_val.tv_usec = time_out % 1000 * 1000;
	int ret = select(socket_fd_ + 1, &read_fds, NULL, NULL, &time_val);
	int result = MSG_IDLE;
	if (ret == -1){
		perror("udp select error\n");
		return result;
	}else if(ret == 0){
		//time out
		*elapsed_time = time_out;
	}else{
		if (FD_ISSET(socket_fd_, &read_fds)){
			*elapsed_time = time_out - (time_val.tv_sec * 1000 + time_val.tv_usec / 1000);
			result = MSG_ARRIVAL;
		}
	}
	return result;
}

