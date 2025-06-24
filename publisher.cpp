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
	pub->RecvData();
	return (void*) 0;
}
void* RecvInnerMsgFunc(void* param) {
	//cout << "the thread no using" << endl;
	Publisher* pub = (Publisher*) param;
	pub->RecvInnerMsg();
	return (void*) 0;
}

Publisher::Publisher() {
	inner_com_socket_ = new UdpSocket();
	recv_socket_ = new UdpSocket();
//	printfNum = new PrintfNum();
}

Publisher::~Publisher() {
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

	if (!inner_com_socket_->Create()) {
		return false;
	}

	if (!inner_com_socket_->Bind("0.0.0.0", com_bind_port)) {
		return false;
	}

	if (!recv_socket_->Create()) {
		return false;
	}

	if (!recv_socket_->Bind("0.0.0.0", recv_port)) {
		return false;
	}

	AddTransaction(60000);   // temporary  video transaction id 10000
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

	if (!inner_com_socket_->Create()) {
		return false;
	}

	if (!inner_com_socket_->Bind("0.0.0.0", com_bind_port)) {
		return false;
	}
	if (!recv_socket_->Create()) {
		return false;
	}

	if (!recv_socket_->Bind("0.0.0.0", recv_port)) {
		return false;
	}
	return true;
}

void Publisher::StartWork() {
	work_over_ = false;
	StartThread(&recv_thread_, RecvDataFunc, this);
	StartThread(&inner_com_thread_, RecvInnerMsgFunc, this);
}

void Publisher::RecvData() {
	const int kMaxBufSize = 65000;
	const int kIdSize = 4;
	char recv_buffer[kMaxBufSize];
	int recv_len;
	char sendbuf[] = { 1, 2, 3, 4, 5 };
	//recv_socket_->SendTo(sendbuf, 5, );
	uint32_t trans_id;
	while (!work_over_) {
		recv_len = recv_socket_->Recv(recv_buffer, kMaxBufSize);
		//printf("recv_len:%d\n",recv_len);
		if (recv_len <= 0) {
			continue;
		}
//		printfNum->setRecvNum();
		++recvNum;

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
			char tmp_str[200];
			sprintf(tmp_str,
					"recv data with trans_id %d, but this trans_id doesn't exist in worker map",
					trans_id);
			g_logger.LogToFile(tmp_str);
		} else {
			worker_map_[trans_id]->RecvPacket(recv_buffer, recv_len);
		}
	}
}

void Publisher::StopWork() {
	StopThread(recv_thread_);
	//delete all work thread
	PubWkMapIt it = worker_map_.begin();
	while (it != worker_map_.end()) {
		delete it->second;
		++it;
	}
}

void Publisher::AddTransaction(uint32_t trans_id) {
	if (ExistTrans(trans_id)) {
		char tmp_str[200];
		sprintf(tmp_str,
				"try to add transaction with trans_id %d, but this transaction already exists in map",
				trans_id);
		g_logger.LogToFile(tmp_str);
		return;
	}

	cout << setw(20) << "pub_type " << pub_type_ << endl;
	PubWorker* pub_worker = new PubWorker(pub_type_);
	pub_worker->Init(trans_id, cell_size_, dst_port_, dst_ip_);
	pub_worker->StartWork();
	worker_map_[trans_id] = pub_worker;
}

void Publisher::DelTransaction(uint32_t trans_id) {
	if (!ExistTrans(trans_id)) {
		char tmp_str[200];
		sprintf(tmp_str,
				"try to del transaction with trans_id %d, but this transaction doesn't exist in map",
				trans_id);
		g_logger.LogToFile(tmp_str);
		return;
	}
	delete worker_map_[trans_id];
	worker_map_.erase(trans_id);
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
	while (!work_over_) {
		recv_len = inner_com_socket_->RecvFrom(recv_buffer, kMaxBufSize,
				&src_addr);
		uint8_t signal_type = *(uint8_t*) recv_buffer;
		if (signal_type == 0x10) {
			int cell_length = *(int*) (recv_buffer + 2);
			cell_size_ = cell_length;
			printf("set cell_size%d\n", cell_length);
			//gParameter.cell_length = cell_length;
			//gParameter.SaveConfig();
			PubWkMapIt it = worker_map_.begin();
			while (it != worker_map_.end()) {
				it->second->SetCellSize(cell_length);
				++it;
			}
		}
	}
}
