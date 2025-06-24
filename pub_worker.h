#ifndef PUB_WORKER_H_
#define PUB_WORKER_H_
#include<pthread.h>
#include<stdint.h>
#include<map>
#include"utils.h"
#include<sys/time.h>

extern int sleep_num;
//20220919
extern pthread_cond_t cond;
extern pthread_mutex_t mutex;

#define MIN_SIZE_TO_HOLD_NEXT_PACK 30// the minimum blank size of cell to hold a new packet (or part of new packet)
enum PubType {
	FILE_PUB = 1, MSG_PUB = 2, IPSTREAM_PUB = 3, VIDEO_PUB = 4
};

class UdpSocket;
class FifoQueue;
class PubWorker {
	friend void* ProcsFunc(void* param);
public:
	PubWorker(int type);
	~PubWorker();

	const int pub_type() const {
		return pub_type_;
	}

	void Init(uint32_t trans_id, int cell_size, int dst_port, char* dst_ip);
	void LoadLevelMap();
	void StartWork();
	void RecvPacket(char* packet, int len);
	void SetCellSize(int cell_size);

	typedef std::map<int, int> DEVINZQMap;
private:
	void StopProcThrd();
	void ProcsMsg();
	void ProcsVideo();
	void CheckAndSndCell(char* cell_buffer, bool* has_data_in_cell);
	int PackCompleteMsgPack(int msg_type, char* cell_buffer, char*& cell_pos,
			bool* has_data_in_cell);
	//build type:
	//0x00   whole packet
	//0x01   first part of a cell
	//0x02   second part of a cell
	void PackVideoCell(char* cell_buffer, int cell_type, int build_type,
			uint16_t data_len, char*& cell_pos, char*& data_pos,
			uint16_t* left_pack_size);

	bool PackMsgPacketHead(int pack_part_type, int msg_type, int len,
			uint16_t* frame_head);

	FifoQueue* fifo_queue_;
	UdpSocket* send_socket_;
	struct sockaddr_in target_addr_;

	pthread_t proc_thread_;
	int pub_type_;
	uint32_t trans_id_;
	int cell_size_;

	uint16_t security_level_;
	bool work_over_;
	DEVINZQMap devinZQ_map_;

};
#endif
