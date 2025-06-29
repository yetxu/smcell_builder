#include<string.h>
#include<assert.h>
#include"publisher.h"
#include"udp_socket.h"
#include"pub_worker.h"
#include"parameter.h"
#include"utils.h"
#include"logger.h"
#include"json/writer.h"
#include"json/reader.h"
#include"signal_define.h"
extern uint32_t recvNum;

extern Logger g_logger;
extern Parameter gParameter;
using namespace std;

void* RecvDataFunc(void* param) {
	Publisher* pub = (Publisher*) param;
	LOG_INFO("Starting receive data thread");
	pub->RecvData();
	LOG_INFO("Receive data thread stopped");
	return (void*) 0;
}
void* RecvInnerMsgFunc(void* param) {
	//cout << "the thread no using" << endl;
	Publisher* pub = (Publisher*) param;
	LOG_INFO("Starting inner communication thread");
	pub->RecvInnerMsg();
	LOG_INFO("Inner communication thread stopped");
	return (void*) 0;
}

Publisher::Publisher() {
	inner_com_socket_ = new UdpSocket();
	recv_socket_ = new UdpSocket();
//	printfNum = new PrintfNum();
	LOG_DEBUG("Publisher object created");
}

Publisher::~Publisher() {
	LOG_DEBUG("Publisher object destroyed");
	StopWork();
	delete recv_socket_;
	delete inner_com_socket_;
}
bool Publisher::Init(Parameter* parameter) {

	gettimeofday(&start_time_, NULL);
	pub_type_ = VIDEO_PUB;
	int recv_port = parameter->radar_recv_port; //32001
	int com_bind_port = parameter->inner_radar_port; //32002
	dst_port_ = parameter->dst_port;
	strcpy(dst_ip_, parameter->dst_ip);
	cell_size_ = parameter->cell_length;   //491

	LOG_INFO("Initializing Publisher with parameters:");
	LOG_INFO("  - Receive port: %d", recv_port);
	LOG_INFO("  - Inner communication port: %d", com_bind_port);
	LOG_INFO("  - Destination IP: %s", dst_ip_);
	LOG_INFO("  - Destination port: %d", dst_port_);
	LOG_INFO("  - Cell size: %d", cell_size_);

	if (!inner_com_socket_->Create()) {
		LOG_ERROR("Failed to create inner communication socket");
		return false;
	}

	if (!inner_com_socket_->Bind("0.0.0.0", com_bind_port)) {
		LOG_ERROR("Failed to bind inner communication socket to port %d", com_bind_port);
		return false;
	}

	if (!recv_socket_->Create()) {
		LOG_ERROR("Failed to create receive socket");
		return false;
	}

	if (!recv_socket_->Bind("0.0.0.0", recv_port)) {
		LOG_ERROR("Failed to bind receive socket to port %d", recv_port);
		return false;
	}

	AddTransaction(60000);   // temporary  video transaction id 10000
	LOG_INFO("Publisher initialization completed successfully");
	return true;
}
bool Publisher::Init(int argc, char* argv[]) {
	assert(argc == 7);
	gettimeofday(&start_time_, NULL);
	pub_type_ = atoi(argv[1]);
	int recv_port = atoi(argv[2]);
	int com_bind_port = atoi(argv[3]);
	dst_port_ = atoi(argv[4]);
	strcpy(dst_ip_, argv[5]);
	cell_size_ = atoi(argv[6]);

	LOG_INFO("Initializing Publisher with command line arguments");
	LOG_INFO("  - Publish type: %d", pub_type_);
	LOG_INFO("  - Receive port: %d", recv_port);
	LOG_INFO("  - Inner communication port: %d", com_bind_port);
	LOG_INFO("  - Destination IP: %s", dst_ip_);
	LOG_INFO("  - Destination port: %d", dst_port_);
	LOG_INFO("  - Cell size: %d", cell_size_);

	if (!inner_com_socket_->Create()) {
		LOG_ERROR("Failed to create inner communication socket");
		return false;
	}

	if (!inner_com_socket_->Bind("0.0.0.0", com_bind_port)) {
		LOG_ERROR("Failed to bind inner communication socket to port %d", com_bind_port);
		return false;
	}
	if (!recv_socket_->Create()) {
		LOG_ERROR("Failed to create receive socket");
		return false;
	}
	if (!recv_socket_->Bind("0.0.0.0", recv_port)) {
		LOG_ERROR("Failed to bind receive socket to port %d", recv_port);
		return false;
	}
	LOG_INFO("Publisher initialization completed successfully");
	return true;
}

void Publisher::StartWork() {
	LOG_INFO("Starting Publisher work threads");
	work_over_ = false;
	StartThread(&recv_thread_, RecvDataFunc, this);
	StartThread(&inner_com_thread_, RecvInnerMsgFunc, this);
	LOG_INFO("Publisher work threads started");
}

void Publisher::RecvData() {
	const int kMaxBufSize = 65000;
	const int kIdSize = 4;
	char recv_buffer[kMaxBufSize];
	int recv_len;
	char sendbuf[] = { 1, 2, 3, 4, 5 };
	//recv_socket_->SendTo(sendbuf, 5, );
	uint32_t trans_id;
	LOG_INFO("Entering receive data loop");
	while (!work_over_) {
		recv_len = recv_socket_->Recv(recv_buffer, kMaxBufSize);
		//printf("recv_len:%d\n",recv_len);
		if (recv_len <= 0) {
			continue;
		}
//		printfNum->setRecvNum();
		++recvNum;

		// 记录网络接收日志
		LOG_NETWORK("UDP_RECV", "0.0.0.0", 0, recv_len);

		//20220919
		//printf("sleep_num = %d\n", sleep_num);
		if (sleep_num) {
			//printf("unlock\n");
			pthread_mutex_lock(&mutex);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
		}

		// trans_id = ntohl(*(uint32_t*)recv_buffer);
		trans_id = 60000;
		if (!ExistTrans(trans_id)) {
			LOG_ERROR("Transaction ID %d not found in worker map", trans_id);
		} else {
			worker_map_[trans_id]->RecvPacket(recv_buffer, recv_len);
		}
	}
	LOG_INFO("Exiting receive data loop");
}

void Publisher::StopWork() {
	LOG_INFO("Stopping Publisher work");
	StopThread(recv_thread_);
	//delete all work thread
	PubWkMapIt it = worker_map_.begin();
	while (it != worker_map_.end()) {
		LOG_DEBUG("Deleting worker for transaction ID %d", it->first);
		delete it->second;
		++it;
	}
	LOG_INFO("Publisher work stopped");
}

void Publisher::AddTransaction(uint32_t trans_id) {
	if (ExistTrans(trans_id)) {
		LOG_WARN("Transaction ID %d already exists in worker map", trans_id);
		return;
	}

	LOG_INFO("Adding transaction ID %d with publish type %d", trans_id, pub_type_);
	cout << setw(20) << "pub_type " << pub_type_ << endl;
	PubWorker* pub_worker = new PubWorker(pub_type_);
	pub_worker->Init(trans_id, cell_size_, dst_port_, dst_ip_);
	pub_worker->StartWork();
	worker_map_[trans_id] = pub_worker;
	LOG_INFO("Transaction ID %d added successfully", trans_id);
}

void Publisher::DelTransaction(uint32_t trans_id) {
	if (!ExistTrans(trans_id)) {
		LOG_WARN("Transaction ID %d not found in worker map for deletion", trans_id);
		return;
	}
	LOG_INFO("Deleting transaction ID %d", trans_id);
	delete worker_map_[trans_id];
	worker_map_.erase(trans_id);
	LOG_INFO("Transaction ID %d deleted successfully", trans_id);
}

bool Publisher::ExistTrans(uint32_t trans_id) {
	return worker_map_.find(trans_id) != worker_map_.end();
}

uint32_t Publisher::GetWorkTime() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec - start_time_.tv_sec;
}

void Publisher::RecvInnerMsg() {
	const int kMaxBufSize = 10;
	char recv_buffer[kMaxBufSize];
	int recv_len;
	char* recv_pos;
	struct sockaddr_in src_addr;
	LOG_INFO("Entering inner message receive loop");
	while (!work_over_) {
		recv_len = inner_com_socket_->RecvFrom(recv_buffer, kMaxBufSize,
				&src_addr);
		uint8_t signal_type = *(uint8_t*) recv_buffer;
		if (signal_type == 0x10) {
			int cell_length = *(int*) (recv_buffer + 2);
			cell_size_ = cell_length;
			LOG_INFO("Received cell size update: %d", cell_length);
			//gParameter.cell_length = cell_length;
			//gParameter.SaveConfig();
			PubWkMapIt it = worker_map_.begin();
			while (it != worker_map_.end()) {
				it->second->SetCellSize(cell_length);
				++it;
			}
			LOG_INFO("Updated cell size for all workers");
		}
	}
	LOG_INFO("Exiting inner message receive loop");
}


