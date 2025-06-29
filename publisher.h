#ifndef PUBLISHER_H_
#define PUBLISHER_H_
#include<pthread.h>
#include<map>
#include<stdint.h>
#include<sys/time.h>
#include"utils.h"
//#include"PrintfNum.h"
class Parameter;
class UdpSocket;
class PubWorker;
class Publisher {
public:
	Publisher();
	~Publisher();
	bool Init(int argc, char* argv[]);
	bool Init(Parameter* parameter);
	void StartWork();
	void RecvData();
	void RecvInnerMsg();
	
	pthread_t recv_thread() {
		return recv_thread_;
	}
	pthread_t inner_com_thread() {
		return inner_com_thread_;
	}

//private:
	void StopWork();
	void AddTransaction(uint32_t trans_id);
	void DelTransaction(uint32_t trans_id);
	bool ExistTrans(uint32_t trans_id);
	uint32_t GetWorkTime();

	typedef std::map<uint32_t, PubWorker*> PubWkMap;
	typedef PubWkMap::iterator PubWkMapIt;
	enum {
		MAX_IP_SIZE = 16
	};

	/*enum{
	 PUB_CENTER_SERVER = 0,
	 ACCESS_SERVER = 1,
	 VIDEO_PUB_SERVER = 2,
	 MSG_PUB_SERVER = 3,
	 FILE_PUB_SERVER = 4,
	 FILE_STORE_SERVER = 5,
	 MONITOR_SERVER = 6
	 };*/
	enum {
		SET_CELL_SIZE = 0,
		GET_CELL_SIZE = 1,
		SET_IP_ADDRESS = 2,
		GET_IP_ADDRESS = 3,
		GET_WORK_TIME = 4,
		GET_WORK_STATUS = 5
	};

	UdpSocket* recv_socket_;
	UdpSocket* inner_com_socket_;

	pthread_t recv_thread_;
	pthread_t inner_com_thread_;

	int pub_type_; //denote that this publisher is video or msg pub
	int dst_port_;
	char dst_ip_[MAX_IP_SIZE];
	int cell_size_;

	bool work_over_;

	struct timeval start_time_;

	PubWkMap worker_map_;
//	PrintfNum *printfNum;
};
#endif
