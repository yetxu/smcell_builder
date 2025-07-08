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
#include"network_manager.h"
#include"CRC_16.h"
#include"cell_factory.h"
#include<vector>
#include<string>
#include<arpa/inet.h>
#include<netinet/in.h>
extern uint32_t recvNum;

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
	signaling_send_socket_ = new UdpSocket();
	//printfNum = new PrintfNum();
	LOG_DEBUG("Publisher object created");
	// 新增：初始化信令包发送socket
	signaling_send_socket_->Create();
	//初始化 target_addr_（这里需要等Init时设置dst_ip_和dst_port_）
}

Publisher::~Publisher() {
	LOG_DEBUG("Publisher object destroyed");
	StopWork();
	delete recv_socket_;
	delete inner_com_socket_;
	delete signaling_send_socket_;
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

	// 初始化目标地址
	target_addr_.sin_family = AF_INET;
	target_addr_.sin_port = htons(dst_port_);
	target_addr_.sin_addr.s_addr = inet_addr(dst_ip_);

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

	// 初始化目标地址
	target_addr_.sin_family = AF_INET;
	target_addr_.sin_port = htons(dst_port_);
	target_addr_.sin_addr.s_addr = inet_addr(dst_ip_);

	AddTransaction(60000);   // temporary  video transaction id 10000
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
			pthread_mutex_lock(&::mutex);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&::mutex);
		}

		// 新增：解析包类型并处理
		ProcessPacket(recv_buffer, recv_len);
	}
	LOG_INFO("Exiting receive data loop");
}

// 新增：包处理主函数
void Publisher::ProcessPacket(char* packet, int len) {
	if (len < 1) {
		LOG_ERROR("Packet too short, cannot determine type");
		return;
	}

	uint8_t packet_type = *(uint8_t*)packet;
	LOG_DEBUG("Received packet type: 0x%02X, length: %d", packet_type, len);

	switch (packet_type) {
		case PACKET_TYPE_NETWORK:
		case PACKET_TYPE_SERIAL:
		case PACKET_TYPE_OTHER:
			ProcessNormalPacket(packet, len, static_cast<PacketType>(packet_type));
			break;
		case PACKET_TYPE_REGISTER:
			ProcessRegisterPacket(packet, len);
			break;
		default:
			LOG_WARN("Unknown packet type: 0x%02X", packet_type);
			break;
	}
}

// 新增：处理普通数据包（网口、串口、其它数据）
void Publisher::ProcessNormalPacket(char* packet, int len, PacketType type) {
	// 跳过包类型字节，处理剩余数据
	char* data = packet + 1;
	int data_len = len - 1;
	
	if (data_len <= 0) {
		LOG_ERROR("Normal packet data too short");
		return;
	}

	LOG_DEBUG("Processing normal packet type: 0x%02X, data length: %d", type, data_len);

	// 使用原有的处理逻辑
	uint32_t trans_id = 60000; // 临时使用固定transaction ID
	if (!ExistTrans(trans_id)) {
		LOG_ERROR("Transaction ID %d not found in worker map", trans_id);
		return;
	}
	
	// 将数据传递给worker处理
	worker_map_[trans_id]->RecvPacket(data, data_len);
}

// 新增：处理注册信令包
void Publisher::ProcessRegisterPacket(char* packet, int len) {
	if (len < 2) {
		LOG_ERROR("Register packet too short for subtype");
		return;
	}
	uint8_t subtype = (uint8_t)packet[1];
	switch (subtype) {
		case 0x01:
			HandleRegisterCellResponse(packet, len);
			break;
		case 0x02:
			HandleReloadNetworkFromDB();
			break;
		case 0x03:
			HandleReloadNetworkFromDB();
			HandleRegisterCellResponse(packet, len);
			break;
		default:
			LOG_WARN("Unknown register packet subtype: 0x%02X", subtype);
			break;
	}
}

void Publisher::HandleRegisterCellResponse(char* packet, int len) {
	// 注册信令包格式：1字节标识 + 1字节子类型 + 4字节sx站IP
	const int REGISTER_PACKET_MIN_LEN = 6;
	if (len < REGISTER_PACKET_MIN_LEN) {
		LOG_ERROR("Register cell response packet too short: %d bytes, expected at least %d", len, REGISTER_PACKET_MIN_LEN);
		return;
	}
	// 提取sx站IP（4字节，从packet[2]开始）
	uint32_t sx_ip = *(uint32_t*)(packet + 2);
	LOG_INFO("Processing register cell response for sx station IP: %u.%u.%u.%u", 
		(sx_ip >> 24) & 0xFF, (sx_ip >> 16) & 0xFF, (sx_ip >> 8) & 0xFF, sx_ip & 0xFF);

	// 将IP转换为字符串格式
	char ip_str[16];
	snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", 
		(sx_ip >> 24) & 0xFF, (sx_ip >> 16) & 0xFF, (sx_ip >> 8) & 0xFF, sx_ip & 0xFF);

	// 查询该IP对应的标签列表
	std::vector<std::string> labels;
	std::string label = g_network_manager.GetLabelByIP(ip_str);
	if (!label.empty()) {
		labels.push_back(label);
		LOG_INFO("Found label '%s' for IP %s", label.c_str(), ip_str);
	} else {
		LOG_WARN("No label found for IP %s", ip_str);
	}

	// 构建信令Cell包
	char cell_buffer[MAX_CELL_SIZE];
	SignalingPacketInfo sig_info;
	sig_info.sx_ip = sx_ip;
	sig_info.labels = labels;
	sig_info.security_level = 0xFB31;
	sig_info.business_id = 60000;
	
	if (BuildCellPacket(CELL_TYPE_SIGNALING, &sig_info, cell_buffer)) {
		// 发送信令Cell包
		SendCell(cell_buffer, cell_size_);
	} else {
		LOG_ERROR("Failed to build cell for IP %s", ip_str);
	}
}

void Publisher::HandleReloadNetworkFromDB() {
	LOG_INFO("Reloading network relations from database...");
	if (g_network_manager.ReloadNetworkRelations()) {
		LOG_INFO("Network relations reloaded successfully from database");
	} else {
		LOG_ERROR("Failed to reload network relations from database");
	}
}

// 新增：统一Cell包构建函数
bool Publisher::BuildCellPacket(CellType cell_type, const void* packet_info, char* cell_buffer) {
	if (!cell_buffer) {
		LOG_ERROR("Cell buffer is null");
		return false;
	}

	// 清空cell buffer
	memset(cell_buffer, 0, cell_size_);

	switch (cell_type) {
		case CELL_TYPE_SIGNALING:
			return BuildSignalingCellPacket(*(const SignalingPacketInfo*)packet_info, cell_buffer);
		case CELL_TYPE_DATA:
			return BuildDataCellPacket(*(const DataPacketInfo*)packet_info, cell_buffer);
		default:
			LOG_ERROR("Unknown cell type: %d", cell_type);
			return false;
	}
}

// 新增：构建信令Cell包（491字节固定格式）
// 修复 BuildSignalingCellPacket 函数定义
bool Publisher::BuildSignalingCellPacket(const SignalingPacketInfo& info, char* cell_buffer) {
    if (!cell_buffer) {
        LOG_ERROR("Cell buffer is null");
        return false;
    }

    // 检查标签数量
    if (info.labels.size() > 255) {
        LOG_ERROR("Too many labels: %zu, maximum is 255", info.labels.size());
        return false;
    }

    char* pos = cell_buffer;

    // 1. 安全控制头（11字节）
    *(uint16_t*)pos = htons(info.security_level);  // 安全级别（2字节）
    pos += 2;
    *pos = 0x08;  // 业务类型（1字节）- 0x08（信令包）
    pos += 1;
    *(uint32_t*)pos = htonl(info.business_id);  // 业务ID（4字节）
    pos += 4;
    *pos = 0x00;  // 填充（1字节）
    pos += 1;
    pos += 3;  // 填充（3字节）

    // 2. Header部分（7字节）
    *pos = 0x08;  // 1字节 0x08
    pos += 1;
    *(uint32_t*)pos = htonl(info.business_id);  // 4字节 业务Id
    pos += 4;
    *pos = 0x00;  // 1字节 0x00
    pos += 1;

    // 3. Data部分
    *pos = 0x00;  // 1字节 占位
    pos += 1;

    // 标记信息（2字节）- 前3位为001（信令包），后8位标签数量
    uint16_t mark_info = 0x2000;  // 00100000 00000000 (前3位=001)
    mark_info |= (info.labels.size() & 0xFF);  // 后8位=标签数量
    *(uint16_t*)pos = htons(mark_info);
    pos += 2;

    // 标签（每个标签2字节）
    for (size_t i = 0; i < info.labels.size(); ++i) {
        uint16_t label_value = 0;
        if (!info.labels[i].empty()) {
            label_value = static_cast<uint16_t>(atoi(info.labels[i].c_str()));
        }
        *(uint16_t*)pos = htons(label_value);
        pos += 2;
    }

    // Cell body - 插入sx站IP（4字节）
    *(uint32_t*)pos = htonl(info.sx_ip);
    pos += 4;

    // 剩余部分补0
    int remaining = cell_size_ - (pos - cell_buffer) - 2;  // 减去CRC16的2字节
    if (remaining > 0) {
        memset(pos, 0, remaining);
        pos += remaining;
    }

    // CRC16校验（2字节）
    uint16_t checksum = CRC_16(0, (uint8_t*)cell_buffer + 11, cell_size_ - 11 - 2);
    *(uint16_t*)pos = htons(checksum);

    LOG_DEBUG("Built signaling cell with %zu labels for IP %u", info.labels.size(), info.sx_ip);
    return true;
}

// 新增：构建数据Cell包（491字节固定格式）
bool Publisher::BuildDataCellPacket(const DataPacketInfo& info, char* cell_buffer) {
	if (!cell_buffer) {
		LOG_ERROR("Cell buffer is null");
		return false;
	}

	char* pos = cell_buffer;

	// 1. 安全控制头（11字节）
	*(uint16_t*)pos = htons(0xFB31);  // 安全级别（2字节）
	pos += 2;
	*pos = 0x01;  // 业务类型（1字节）- 0x01（数据包）
	pos += 1;
	*(uint32_t*)pos = htonl(60000);  // 业务ID（4字节）
	pos += 4;
	*pos = 0x00;  // 填充（1字节）
	pos += 1;
	pos += 3;  // 填充（3字节）

	// 2. Header部分（7字节）
	*pos = 0x01;  // 1字节 0x01
	pos += 1;
	*(uint32_t*)pos = htonl(60000);  // 4字节 业务Id
	pos += 4;
	*pos = 0x00;  // 1字节 0x00
	pos += 1;

	// 3. Data部分
	*pos = 0x00;  // 1字节 占位
	pos += 1;

	// 标记信息（2字节）- 根据数据长度确定类型
	uint16_t mark_info = 0;
	if (info.data_length <= 66) {
		// 010 集装箱式组包一(x≤66) (3bit 集装箱数量、7bit 集装箱长度)
		mark_info = 0x4000;  // 01000000 00000000
		mark_info |= ((info.container_count & 0x07) << 7);  // 3bit 集装箱数量
		mark_info |= (info.data_length & 0x7F);  // 7bit 集装箱长度
	} else if (info.data_length <= 93) {
		// 011 集装箱式组包二(67≤x≤93) (3bit 集装箱数量、7bit集装箱长度)
		mark_info = 0x6000;  // 01100000 00000000
		mark_info |= ((info.container_count & 0x07) << 7);  // 3bit 集装箱数量
		mark_info |= (info.data_length & 0x7F);  // 7bit 集装箱长度
	} else if (info.data_length <= 155) {
		// 100 集装箱式组包三(94≤x≤155) (2bit 集装箱数量，8bit集装箱长度)
		mark_info = 0x8000;  // 10000000 00000000
		mark_info |= ((info.container_count & 0x03) << 8);  // 2bit 集装箱数量
		mark_info |= (info.data_length & 0xFF);  // 8bit 集装箱长度
	} else if (info.data_length <= 233) {
		// 101 集装箱式组包四(156≤x≤233) (1bit集装箱数量，8bit集装箱长度)
		mark_info = 0xA000;  // 10100000 00000000
		mark_info |= ((info.container_count & 0x01) << 8);  // 1bit 集装箱数量
		mark_info |= (info.data_length & 0xFF);  // 8bit 集装箱长度
	} else if (info.data_length <= 467) {
		// 110 不组包（234≤x≤467）(9bit 标志数据长度)
		mark_info = 0xC000;  // 11000000 00000000
		mark_info |= (info.data_length & 0x1FF);  // 9bit 标志数据长度
	} else {
		// 111 拆包分片发送（x≥468）
		// 这里简化处理，实际应该根据包ID判断头包/中间包/尾包
		if (info.packet_id == 0) {
			// 头包（01. 11bit拆包数量）
			mark_info = 0xE000;  // 11100000 00000000
			mark_info |= ((info.total_packets & 0x7FF) << 5);  // 11bit 拆包数量
		} else if (info.packet_id < info.total_packets - 1) {
			// 中间包（10. 11bit包ID）
			mark_info = 0xE800;  // 11101000 00000000
			mark_info |= ((info.packet_id & 0x7FF) << 5);  // 11bit 包ID
		} else {
			// 尾包（11. 9bit尾包长度）
			mark_info = 0xF000;  // 11110000 00000000
			mark_info |= (info.data_length & 0x1FF);  // 9bit 尾包长度
		}
	}
	*(uint16_t*)pos = htons(mark_info);
	pos += 2;

	// 标签（2字节）
	*(uint16_t*)pos = htons(info.label);
	pos += 2;

	// Cell body - 数据体
	if (info.data_ptr && info.data_length > 0) {
		int max_data_size = cell_size_ - (pos - cell_buffer) - 2;  // 减去CRC16的2字节
		int copy_size = (info.data_length < max_data_size) ? info.data_length : max_data_size;
		memcpy(pos, info.data_ptr, copy_size);
		pos += copy_size;
	}

	// 剩余部分补0
	int remaining = cell_size_ - (pos - cell_buffer) - 2;  // 减去CRC16的2字节
	if (remaining > 0) {
		memset(pos, 0, remaining);
		pos += remaining;
	}

	// CRC16校验（2字节）
	uint16_t checksum = CRC_16(0, (uint8_t*)cell_buffer + 11, cell_size_ - 11 - 2);
	*(uint16_t*)pos = htons(checksum);

	LOG_DEBUG("Built data cell with label %u, data length %u", info.label, info.data_length);
	return true;
}

bool Publisher::SendCell(const char* cell_buffer, int cell_size) {
	if (!signaling_send_socket_) {
		LOG_ERROR("Signaling send socket is not initialized");
		return false;
	}

	int send_result = signaling_send_socket_->SendTo(const_cast<char*>(cell_buffer), cell_size, target_addr_);
	if (send_result > 0) {
		LOG_INFO("Successfully sent cell to %s:%d", dst_ip_, dst_port_);
		LOG_NETWORK("UDP_SEND_CELL", dst_ip_, dst_port_, send_result);
		return true;
	} else {
		LOG_ERROR("Failed to send cell to %s:%d", dst_ip_, dst_port_);
		return false;
	}
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
	pub_worker->SetPublisher(this);
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

// 更新原有的BuildSignalingCell函数，使用新的构建函数
bool Publisher::BuildSignalingCell(uint32_t sx_ip, const std::vector<std::string>& labels, char* cell_buffer) {
	SignalingPacketInfo info;
	info.sx_ip = sx_ip;
	info.labels = labels;
	info.security_level = 0xFB31;
	info.business_id = 60000;

	return BuildSignalingCellPacket(info, cell_buffer);
}

/*
使用示例：

1. 构建信令包：
   SignalingPacketInfo sig_info;
   sig_info.sx_ip = inet_addr("192.168.1.100");
   sig_info.labels.push_back("123");
   sig_info.labels.push_back("456");
   sig_info.security_level = 0xFB31;
   sig_info.business_id = 60000;
   
   char cell_buffer[MAX_CELL_SIZE];
   if (BuildCellPacket(CELL_TYPE_SIGNALING, &sig_info, cell_buffer)) {
       SendCell(cell_buffer, cell_size_);
   }

2. 构建数据包：
   DataPacketInfo data_info;
   data_info.label = 123;
   data_info.data_length = 100;
   data_info.container_count = 2;
   data_info.packet_id = 0;
   data_info.total_packets = 1;
   data_info.data_ptr = your_data_buffer;
   
   char cell_buffer[MAX_CELL_SIZE];
   if (BuildCellPacket(CELL_TYPE_DATA, &data_info, cell_buffer)) {
       SendCell(cell_buffer, cell_size_);
   }

Cell包格式（491字节）：
- 安全控制头：11字节
- Header：7字节（0x08/0x01 + 业务ID4字节 + 0x00）
- Data：1字节占位 + 2字节标记信息 + 2字节标签 + n字节数据体 + 2字节CRC16
*/



