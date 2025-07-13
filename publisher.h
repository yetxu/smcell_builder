#ifndef PUBLISHER_H_
#define PUBLISHER_H_
#include<pthread.h>
#include<map>
#include<vector>
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

	// 新增：包类型枚举
	enum PacketType {
		PACKET_TYPE_NETWORK = 0x00,    // 网口数据
		PACKET_TYPE_SERIAL = 0x01,     // 串口数据
		PACKET_TYPE_OTHER = 0x02,      // 其它数据
		PACKET_TYPE_REGISTER = 0x03,   // 注册信令
	};

	// 新增：Cell包类型枚举
	enum CellType {
		CELL_TYPE_SIGNALING = 0x01,    // 信令包
		CELL_TYPE_DATA = 0x02,         // 数据包
	};

	// 新增：数据包标记类型枚举
	enum DataMarkType {
		DATA_MARK_CONTAINER_1 = 0x02,  // 010 集装箱式组包一(x≤66)
		DATA_MARK_CONTAINER_2 = 0x03,  // 011 集装箱式组包二(67≤x≤93)
		DATA_MARK_CONTAINER_3 = 0x04,  // 100 集装箱式组包三(94≤x≤155)
		DATA_MARK_CONTAINER_4 = 0x05,  // 101 集装箱式组包四(156≤x≤233)
		DATA_MARK_NO_PACK = 0x06,      // 110 不组包（234≤x≤467）
		DATA_MARK_SPLIT_HEAD = 0x07,   // 111 拆包分片发送-头包
		DATA_MARK_SPLIT_MIDDLE = 0x08, // 111 拆包分片发送-中间包
		DATA_MARK_SPLIT_TAIL = 0x09,   // 111 拆包分片发送-尾包
	};

	// 新增：数据包信息结构体
	struct DataPacketInfo {
		uint16_t label;           // 标签
		uint16_t data_length;     // 数据长度
		uint16_t container_count; // 集装箱数量（用于集装箱式组包）
		uint16_t packet_id;       // 包ID（用于拆包分片）
		uint16_t total_packets;   // 总包数（用于拆包分片）
		char* data_ptr;           // 数据指针
	};

	// 新增：信令包信息结构体
	struct SignalingPacketInfo {
		uint32_t sx_ip;                    // sx站IP
		std::vector<std::string> labels;   // 标签列表
		uint16_t security_level;           // 安全级别
		uint32_t business_id;              // 业务ID
	};

	// 新增：包处理函数
	void ProcessPacket(char* packet, int len);
	void ProcessNormalPacket(char* packet, int len, PacketType type);
	void ProcessRegisterPacket(char* packet, int len);
	void ProcessReloadNetworkPacket();
	bool BuildSignalingCell(uint32_t sx_ip, const std::vector<std::string>& labels, char* cell_buffer);

	// 新增：统一Cell包构建函数
	bool BuildCellPacket(CellType cell_type, const void* packet_info, char* cell_buffer);
	bool BuildSignalingCellPacket(const SignalingPacketInfo& info, char* cell_buffer);
	bool BuildDataCellPacket(const DataPacketInfo& info, char* cell_buffer);
	bool SendCell(const char* cell_buffer, int cell_size);

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
	UdpSocket* signaling_send_socket_;

	pthread_t recv_thread_;
	pthread_t inner_com_thread_;

	int pub_type_; //denote that this publisher is video or msg pub
	int dst_port_;
	char dst_ip_[MAX_IP_SIZE];
	int cell_size_;

	bool work_over_;

	struct timeval start_time_;

	PubWkMap worker_map_;
	struct sockaddr_in target_addr_;
//	PrintfNum *printfNum;

private:
	void HandleRegisterCellResponse(char* packet, int len, uint8_t subtype);
	void HandleReloadNetworkFromDB();
};
#endif
