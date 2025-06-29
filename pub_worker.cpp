#include"pub_worker.h"
#include"utils.h"
#include"udp_socket.h"
#include"fifo_queue.h"
#include"cell_factory.h"
#include"CRC_16.h"
#include"json/reader.h"
#include"json/writer.h"
#include"network_manager.h"
#include<fstream>
#include<iostream>

using namespace std;
using namespace json;

uint32_t recvNum = 0;
uint32_t sendNum = 0;
uint32_t waitNum = 0;
uint32_t splitNum = 0;

//20220919
int sleep_num = 0;
pthread_cond_t cond;
pthread_mutex_t mutex;
#define PEPS_TIMEOUT 10
struct timeval now;
struct timespec outtime;

void* ProcsFunc(void* param) {
	PubWorker* pub_worker = (PubWorker*) param;
	if (pub_worker->pub_type() == MSG_PUB) {
		pub_worker->ProcsMsg();
	} else if (pub_worker->pub_type() == VIDEO_PUB) {
		pub_worker->ProcsVideo();
	}
	return (void*) 0;
}

PubWorker::PubWorker(int type) :
		pub_type_(type), work_over_(true) {
	send_socket_ = new UdpSocket();
	fifo_queue_ = new FifoQueue();

	security_level_ = ntohs(0xFB31);  //security level, how to get ?
	LOG_DEBUG("PubWorker created with type %d", type);
}

PubWorker::~PubWorker() {
	LOG_DEBUG("PubWorker destroyed");
	StopProcThrd();
	delete send_socket_;
	delete fifo_queue_;
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

void PubWorker::LoadLevelMap() {
	Object root;
	//read config from file
	ifstream fin;
	fin.open("./map.json");
	assert(fin.is_open());
	Reader::Read(root, fin);
	fin.close();
	int devID = 0;
	int devLevel = 0;
	char str[100];

	int num = Number(root["sumnum"]).Value();

	LOG_INFO("Loading SE Level Map from map.json");
	cout << setw(20) << "Sumnum " << num << endl;
	for (int i = 1; i < num + 1; i++) {
		memset(str, '\0', 100);
		sprintf(str, "devID%d", i);
		devID = Number(root[str]).Value();
		if (devID < 1 || devID > 10000)
			continue;
		memset(str, '\0', 100);
		sprintf(str, "devLevel%d", i);
		devLevel = Number(root[str]).Value();
		devinZQ_map_[devID] = devLevel;
	}
	DEVINZQMap::iterator it;
	int index = 1;
	cout << setw(10) << "index" << setw(10) << "devID" << setw(10) << "levelID"
			<< endl;
	for (it = devinZQ_map_.begin(); it != devinZQ_map_.end(); it++) {
		cout << setw(20) << index++ << setw(20) << it->first << setw(20)
				<< it->second << endl;
	}
	LOG_INFO("Loaded %zu SE level mappings", devinZQ_map_.size());
}
void PubWorker::Init(uint32_t trans_id, int cell_size, int dst_port,
		char* dst_ip) {
	trans_id_ = trans_id;
	cell_size_ = cell_size;
	send_socket_->Create();
	target_addr_.sin_family = AF_INET;
	target_addr_.sin_port = htons(dst_port);
	target_addr_.sin_addr.s_addr = inet_addr(dst_ip);

	LOG_INFO("Initializing PubWorker:");
	LOG_INFO("  - Transaction ID: %u", trans_id);
	LOG_INFO("  - Cell size: %d", cell_size);
	LOG_INFO("  - Destination: %s:%d", dst_ip, dst_port);

	//init devinZQ map
	//devinZQ_map_[38]=29;
	//printf("map num=%d\n",devinZQ_map_.size());
	LoadLevelMap();
//    send_socket_->Connect(dst_ip, dst_port);

	//20220919
	if (pthread_mutex_init(&mutex, NULL)) {
		LOG_ERROR("Failed to create mutex");
	} else {
		LOG_DEBUG("Mutex created successfully");
	}
	if (pthread_cond_init(&cond, NULL)) {
		LOG_ERROR("Failed to create condition variable");
	} else {
		LOG_DEBUG("Condition variable created successfully");
	}
}

void PubWorker::StartWork() {
	LOG_INFO("Starting PubWorker with type %d", pub_type_);
	if (!work_over_) {
		return;
	}
	work_over_ = false;
	StartThread(&proc_thread_, ProcsFunc, this);
	LOG_INFO("PubWorker started successfully");
}

void PubWorker::StopProcThrd() {
	if (work_over_) {
		return;
	}
	LOG_INFO("Stopping PubWorker");
	work_over_ = true;
	StopThread(proc_thread_);
	LOG_INFO("PubWorker stopped");
}

void PubWorker::RecvPacket(char* packet, int len) {
	fifo_queue_->PushPacket(packet, (uint16_t) len);
	LOG_DEBUG("Received packet of length %d", len);
}

void PubWorker::ProcsMsg() {
	const int kMaxBufSize = 6000;
	char data_buf[kMaxBufSize];
	char* data_pos;
	uint16_t data_len;

	const int kCellHeadLen = 17; //header length of msg cell, defaultly, we don't count the unicast ip in it

	char cell_buffer[MAX_CELL_SIZE];

	//the constant content in cell
	char* cell_pos = cell_buffer + SECURITY_LEV_POS;
	*(uint16_t*) cell_pos = security_level_;
	cell_pos += 2;
	*(uint8_t*) cell_pos = MSG_SERVICE;
	cell_pos += 1;
	*(uint32_t*) cell_pos = trans_id_;
	cell_pos += 4;
	*(uint8_t*) cell_pos = 0;
	cell_pos += 1;

	uint16_t next_pack_size;

	int msg_type = SM_TYPE_MESSAGE;

	uint16_t msg_seq = 0;

	uint16_t left_pack_size = 0;

	bool has_data_in_cell = false;
	while (!work_over_) {
		if (!has_data_in_cell) {
			cell_pos = cell_buffer + kCellHeadLen; //start position of msg data in cell buffer
			memset(cell_pos, 0, cell_size_ - kCellHeadLen);

			*(uint16_t*) cell_pos = msg_seq++;
			cell_pos += sizeof(msg_seq);
		}

		if (left_pack_size > 0) {
			uint16_t part_size =
					cell_buffer
							+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE - STOP_TAG_SIZE;

			if (part_size >= left_pack_size) {
				PackMsgPacketHead(MSG_TAIL, msg_type, left_pack_size,
						(uint16_t*) cell_pos);
				cell_pos += FRAME_HEAD_SIZE;
				memcpy(cell_pos, data_pos, left_pack_size);
				cell_pos += left_pack_size;
				has_data_in_cell = true;
				data_pos += left_pack_size;
				part_size -= left_pack_size;
				left_pack_size = 0;
			} else {
				PackMsgPacketHead(MSG_BODY, msg_type, part_size,
						(uint16_t*) cell_pos);
				cell_pos += FRAME_HEAD_SIZE;
				memcpy(cell_pos, data_pos, part_size);
				cell_pos += part_size;
				has_data_in_cell = true;
				data_pos += part_size;
				left_pack_size -= part_size;
				part_size = 0;
			}

			//if we have more than MIN_SIZE_TO_HOLD_NEXT_PACK space left in cell, instead of sending cell over ,
			//try to add more msg data into cell , this case only occures when cell load tail part of a big msg packet
			if (part_size > MIN_SIZE_TO_HOLD_NEXT_PACK) { //if this cell doesn't load the tail of the big packet
														  //this if clause will not be executed because cell that
														  //load body of big packet will be full
				PackCompleteMsgPack(msg_type, cell_buffer, cell_pos,
						&has_data_in_cell);

				if (part_size < MIN_SIZE_TO_HOLD_NEXT_PACK) {
					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				}

				next_pack_size = fifo_queue_->PeekNextPackLen();
				if (next_pack_size == 0) { //fifo queue is empty currently
					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				}

				// left space in cell > MIN_SIZE_TO_HOLD_NEXT_PACK and can read data
				fifo_queue_->PopPacket(data_buf, &left_pack_size);
				data_pos = data_buf;

				part_size =
						cell_buffer
								+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE - STOP_TAG_SIZE;

				if (part_size >= left_pack_size) {
					PackMsgPacketHead(COMPLETE_MSG, msg_type, left_pack_size,
							(uint16_t*) cell_pos);
					cell_pos += FRAME_HEAD_SIZE;
					memcpy(cell_pos, data_pos, left_pack_size);
					cell_pos += left_pack_size;
					data_pos += left_pack_size;
					has_data_in_cell = true;
					part_size -= left_pack_size;
					left_pack_size = 0;
					if (part_size < MIN_SIZE_TO_HOLD_NEXT_PACK) {
						CheckAndSndCell(cell_buffer, &has_data_in_cell);
					}
					continue;
				} else {
					PackMsgPacketHead(MSG_HEAD, msg_type, part_size,
							(uint16_t*) cell_pos);
					cell_pos += FRAME_HEAD_SIZE;
					memcpy(cell_pos, data_pos, part_size);
					cell_pos += part_size;
					has_data_in_cell = true;
					data_pos += part_size;
					left_pack_size -= part_size;

					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				}

			} else {
				CheckAndSndCell(cell_buffer, &has_data_in_cell);
				continue;
			}
		}

		PackCompleteMsgPack(msg_type, cell_buffer, cell_pos, &has_data_in_cell);

		if (cell_buffer + cell_size_ - cell_pos < MIN_SIZE_TO_HOLD_NEXT_PACK) {
			CheckAndSndCell(cell_buffer, &has_data_in_cell);
			continue;
		}
		next_pack_size = fifo_queue_->PeekNextPackLen();
		if (next_pack_size == 0 && has_data_in_cell) { //fifo queue is empty currently and have data in cell_buffer
			CheckAndSndCell(cell_buffer, &has_data_in_cell);
			continue;
		}

		//no data in cell || buffer is not empty and  left space in cell > MIN_SIZE_TO_HOLD_NEXT_PACK
		fifo_queue_->PopPacket(data_buf, &left_pack_size);
		data_pos = data_buf;

		uint16_t part_size =
				cell_buffer
						+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE - STOP_TAG_SIZE;

		if (part_size >= left_pack_size) {
			PackMsgPacketHead(COMPLETE_MSG, msg_type, left_pack_size,
					(uint16_t*) cell_pos);
			cell_pos += FRAME_HEAD_SIZE;
			memcpy(cell_pos, data_pos, left_pack_size);
			cell_pos += left_pack_size;
			data_pos += left_pack_size;
			has_data_in_cell = true;
			part_size -= left_pack_size;
			left_pack_size = 0;
			if (part_size < MIN_SIZE_TO_HOLD_NEXT_PACK) {
				CheckAndSndCell(cell_buffer, &has_data_in_cell);
				continue;
			}
		} else {
			PackMsgPacketHead(MSG_HEAD, msg_type, part_size,
					(uint16_t*) cell_pos);
			cell_pos += FRAME_HEAD_SIZE;
			memcpy(cell_pos, data_pos, part_size);
			cell_pos += part_size;
			has_data_in_cell = true;
			data_pos += part_size;
			left_pack_size -= part_size;

			CheckAndSndCell(cell_buffer, &has_data_in_cell);
			continue;
		}

	}
}

int PubWorker::PackCompleteMsgPack(int msg_type, char* cell_buffer,
		char*& cell_pos, bool* has_data_in_cell) {
	uint16_t data_len;
	uint16_t next_pack_size = 0;
	int complete_pack_num = 0;
	while (true) {
		next_pack_size = fifo_queue_->PeekNextPackLen();
		if (next_pack_size
				== 0|| next_pack_size + cell_pos + FRAME_HEAD_SIZE > cell_buffer + cell_size_ - CHECK_SUM_SIZE - STOP_TAG_SIZE) {
			break;
		}

		fifo_queue_->PopPacket(cell_pos + FRAME_HEAD_SIZE, &data_len);
		PackMsgPacketHead(COMPLETE_MSG, msg_type, data_len,
				(uint16_t*) cell_pos);
		cell_pos += (data_len + FRAME_HEAD_SIZE);
		complete_pack_num++;
		*has_data_in_cell = true;
	}
	return complete_pack_num;
}

void PubWorker::ProcsVideo() {
	const int kMaxBufSize = 65536;
	char data_buf[kMaxBufSize];   //get the data from fifoqueue
	char* data_pos;
	uint16_t data_len;

	const int kCellHeadLen = 17; //header length of msg cell, defaultly, we don't count the unicast ip in it

	char cell_buffer[MAX_CELL_SIZE];

	//the constant content in cell
	char* cell_pos = cell_buffer + SECURITY_LEV_POS;  //cell_buffer+9
	*(uint16_t*) cell_pos = security_level_;
	cell_pos += 2;                                    //cell_buffer+11
	*(uint8_t*) cell_pos = RADAR_SERVICE;
	cell_pos += 1;                                     //cell_buffer+12
	*(uint32_t*) cell_pos = trans_id_;
	cell_pos += 4;                                    //cell_buffer+16
	*(uint8_t*) cell_pos = 0;
	cell_pos += 1;                                    //cell_buffer+17
	*(uint8_t*) cell_pos = RADAR_SERVICE;

	uint16_t first_pack_tail;

	uint16_t body_length;

	uint16_t next_pack_size;

	uint16_t left_pack_size = 0;              //the data size left from data_buf

	bool has_data_in_cell = false;

	int cell_type;
	while (!work_over_) {
//		cout<<"work_over:"<<work_over_<<endl;
		cell_pos = cell_buffer + kCellHeadLen; //start position of msg data in cell buffer
		memset(cell_pos, 0, cell_size_ - kCellHeadLen);
		*(uint8_t*) cell_pos = RADAR_SERVICE;
		cell_pos += 6;

		if (left_pack_size > 0) { //left pack size > 0, means that we are handling body part (or tail part) of a packet
			++splitNum;
			uint16_t part_size = cell_buffer
					+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE;
			if (part_size <= left_pack_size) {
				if (part_size == left_pack_size) {
					PackVideoCell(cell_buffer, CELL_TYPE_TAIL_ONLY, 0x00,
							part_size, cell_pos, data_pos, &left_pack_size);
				} else {
					PackVideoCell(cell_buffer, CELL_TYPE_BODY, 0x00, part_size,
							cell_pos, data_pos, &left_pack_size);
				}
				CheckAndSndCell(cell_buffer, &has_data_in_cell);
				continue;
			} else {
				if (cell_buffer + cell_size_
						- cell_pos< MIN_SIZE_TO_HOLD_NEXT_PACK) {
					PackVideoCell(cell_buffer, CELL_TYPE_TAIL_ONLY, 0x00,
							left_pack_size, cell_pos, data_pos,
							&left_pack_size);
					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				}

				next_pack_size = fifo_queue_->PeekNextPackLen();
				if (next_pack_size == 0) {
					PackVideoCell(cell_buffer, CELL_TYPE_TAIL_ONLY, 0x00,
							left_pack_size, cell_pos, data_pos,
							&left_pack_size);
					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				} else {
					//pack first part
					PackVideoCell(cell_buffer, CELL_TYPE_UNKNOWN, 0x01,
							left_pack_size, cell_pos, data_pos,
							&left_pack_size);
					fifo_queue_->PopPacket(data_buf, &left_pack_size);
					data_pos = data_buf;
					//pack second part
					part_size =
							cell_buffer
									+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE;
					if (part_size >= left_pack_size) {
						PackVideoCell(cell_buffer, CELL_TYPE_TAIL_ANTH_TAIL,
								0x02, left_pack_size, cell_pos, data_pos,
								&left_pack_size);
					} else {
						PackVideoCell(cell_buffer, CELL_TYPE_TAIL_ANTH, 0x02,
								part_size, cell_pos, data_pos, &left_pack_size);
					}
					CheckAndSndCell(cell_buffer, &has_data_in_cell);
					continue;
				}
			}
		}

		fifo_queue_->PopPacket(data_buf, &left_pack_size); //this may be blocked if queue is empty,
														   //it get data from fifo_queue and create the first cell 
		data_pos = data_buf;
		uint16_t part_size = cell_buffer
				+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE; //part_size is the left size from  the cell
		if (part_size - left_pack_size
				>= 0&& part_size - left_pack_size < MIN_SIZE_TO_HOLD_NEXT_PACK) {
			PackVideoCell(cell_buffer, CELL_TYPE_HEAD_TAIL_ONLY, 0x00,
					left_pack_size, cell_pos, data_pos, &left_pack_size);
			CheckAndSndCell(cell_buffer, &has_data_in_cell);
			continue;
		}
		if (part_size > left_pack_size) { //we need to join together two packets
			//printf("part_size = %d, left_pack_size = %d\n", part_size, left_pack_size);

			//20220919
			sleep_num = 1;
			//printf("lock\n");
			pthread_mutex_lock(&mutex);
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + PEPS_TIMEOUT / 1000;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&cond, &mutex, &outtime);
			// if(ret != 0)
			// {
			//     printf("timeout\n");
			// }
			// else{
			//     printf("wait\n");
			// }
			pthread_mutex_unlock(&mutex);
			sleep_num = 0;

			next_pack_size = fifo_queue_->PeekNextPackLen();
			if (next_pack_size == 0) {
				PackVideoCell(cell_buffer, CELL_TYPE_HEAD_TAIL_ONLY, 0x00,
						left_pack_size, cell_pos, data_pos, &left_pack_size);
				CheckAndSndCell(cell_buffer, &has_data_in_cell);
				continue;
			} else {
				//pack first part
				PackVideoCell(cell_buffer, CELL_TYPE_UNKNOWN, 0x01,
						left_pack_size, cell_pos, data_pos, &left_pack_size);

				fifo_queue_->PopPacket(data_buf, &left_pack_size);

				data_pos = data_buf;
				//pack second part
				uint16_t part_size =
						cell_buffer
								+ cell_size_- cell_pos - FRAME_HEAD_SIZE - CHECK_SUM_SIZE;
				if (part_size >= left_pack_size) {
					PackVideoCell(cell_buffer, CELL_TYPE_HEAD_TAIL_ANTH_TAIL,
							0x02, left_pack_size, cell_pos, data_pos,
							&left_pack_size);
				} else {
					PackVideoCell(cell_buffer, CELL_TYPE_HEAD_TAIL_ANTH, 0x02,
							part_size, cell_pos, data_pos, &left_pack_size);
				}
				CheckAndSndCell(cell_buffer, &has_data_in_cell);
				++waitNum;
				continue;
			}
		} else {  //part_size < cell_buffer
			PackVideoCell(cell_buffer, CELL_TYPE_HEAD, 0x00, part_size,
					cell_pos, data_pos, &left_pack_size);
			CheckAndSndCell(cell_buffer, &has_data_in_cell);
			continue;
		}
	}
}

bool PubWorker::PackMsgPacketHead(int pack_part_type, int msg_type, int len,
		uint16_t* frame_head) {
	*frame_head = 0;
	*frame_head |= (uint16_t(msg_type & 0x07) << 5);
	*frame_head |= (uint16_t(pack_part_type & 0x03) << 3);
	*frame_head |= ((len & 0xFF) << 8);
	*frame_head |= ((len >> 8) & 0x07);
	return true;
}

void PubWorker::PackVideoCell(char* cell_buffer, int cell_type, int build_type,
		uint16_t data_len, char*& cell_pos, char*& data_pos,
		uint16_t* left_pack_size) {
	const int kCellHeadLen = 17;
	char* start_pos = cell_buffer + kCellHeadLen + 1;
	*(uint8_t*) start_pos = cell_type;
	start_pos += 1;
	if (build_type == 0x00 || build_type == 0x01) {
		*(uint16_t*) start_pos = data_len;  //first pack tail
		start_pos += 2;

		*(uint16_t*) start_pos = data_len;  //body length
		start_pos += 2;

		cell_pos = start_pos;
	}
	if (build_type == 0x02) {
		uint16_t first_pack_tail = *(uint16_t*) start_pos;
		*(uint16_t*) (start_pos + 2) = first_pack_tail + data_len; //fill up the body length of pack
	}

	memcpy(cell_pos, data_pos, data_len);
	cell_pos += data_len;
	data_pos += data_len;
	*left_pack_size -= data_len;
}

void PubWorker::CheckAndSndCell(char* cell_buffer, bool* has_data_in_cell) {
	uint16_t devID = *(uint16_t *) (cell_buffer + 24);
	devID = htons(devID);

	
	// 使用网络管理器获取IP对应的标签
	std::string source_ip = std::to_string(devID); // 这里需要根据实际的数据包格式来提取IP
	std::string label = g_network_manager.GetLabelByIP(source_ip);
	
	if (label.empty()) {
		LOG_DEBUG("No label found for device ID %d, using default security level", devID);
		*(uint16_t*) (cell_buffer + 9) = security_level_;
	} else {
		LOG_DEBUG("Found label '%s' for device ID %d", label.c_str(), devID);
		// 这里可以根据标签设置相应的安全级别
		// 暂时使用默认的安全级别
		*(uint16_t*) (cell_buffer + 9) = security_level_;
	}

	uint16_t checksum = CRC_16(0, (uint8_t*) cell_buffer + SECURITY_HEAD_LEN,
			cell_size_ - SECURITY_HEAD_LEN - CHECK_SUM_SIZE);
	*(uint16_t*) (cell_buffer + cell_size_ - CHECK_SUM_SIZE) = checksum;
	
	//加控制头需要去掉注释，并注释第一个send_socket，使用第二个send_socket
	char data_buffer[MAX_CELL_SIZE];
	memset(data_buffer, 0, CONTROL_HEADER_LEN + cell_size_);
	char* data_pos = data_buffer;
	*(uint8_t*) data_pos = 0xE8;     //cell_buffer+0
	data_pos += 1;
	*(uint8_t*) data_pos = 0x03;     //cell_buffer+1
	data_pos += 3;
	*(uint8_t*) data_pos = 0x07;		//cell_buffer+4
	data_pos += 1;
	*(uint8_t*) data_pos = 0x00;		//cell_buffer+5
	data_pos += 1;
	*(uint8_t*) data_pos = 0x04;    //cell_buffer+6
	data_pos += 1;
	*(uint8_t*) data_pos = 0xE8;     //cell_buffer+7
	data_pos += 1;
	*(uint8_t*) data_pos = 0x03;		//cell_buffer+8
	memcpy(data_buffer + CONTROL_HEADER_LEN, cell_buffer, cell_size_);
	
	//send_socket_->SendTo(cell_buffer, cell_size_, target_addr_);
#ifdef TEST
	printf("send data...\n");
#endif
	
	// 发送数据
	int send_result = send_socket_->SendTo(data_buffer, cell_size_ + CONTROL_HEADER_LEN, target_addr_);
	
	// 记录网络发送日志
	if (send_result > 0) {
		LOG_NETWORK("UDP_SEND", inet_ntoa(target_addr_.sin_addr), ntohs(target_addr_.sin_port), send_result);
		LOG_DEBUG("Successfully sent %d bytes to %s:%d", send_result, 
		         inet_ntoa(target_addr_.sin_addr), ntohs(target_addr_.sin_port));
	} else {
		LOG_ERROR("Failed to send data to %s:%d", inet_ntoa(target_addr_.sin_addr), ntohs(target_addr_.sin_port));
	}
	
	*has_data_in_cell = false;
	cout << "recvNum:" << recvNum << ", sendNum:" << ++sendNum << ", waitNum:" << waitNum << ", splitNum:"<< splitNum << endl;
}

void PubWorker::SetCellSize(int s) {
	cell_size_ = s;
}

