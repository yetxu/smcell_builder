#ifndef PUB_WORKER_H_
#define PUB_WORKER_H_
#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include "utils.h"
#include <sys/time.h>
#include "cell_factory.h"

// 线程同步外部变量
extern int sleep_num;
extern pthread_cond_t cond;
extern pthread_mutex_t mutex;

#define MIN_SIZE_TO_HOLD_NEXT_PACK 30
#define MAX_BUFFER_SIZE 1000
#define MAX_MSG_BUFFER_SIZE 1000

// 发布类型
enum PubType {
	MSG_PUB = 0
};


// 构建类型枚举
enum BuildType {
	BUILD_WHOLE = 0x00,    // 完整包
	BUILD_FIRST = 0x01,    // 第一部分
	BUILD_SECOND = 0x02    // 第二部分
};

// 数据包信息结构体
struct PacketInfo {
	uint16_t length;
	char* data;
	uint16_t device_id;
	uint16_t sequence;
	
	PacketInfo() : length(0), data(NULL), device_id(0), sequence(0) {}
	PacketInfo(uint16_t len, char* ptr, uint16_t dev_id, uint16_t seq) 
		: length(len), data(ptr), device_id(dev_id), sequence(seq) {}
};

// Cell构建上下文结构体
struct CellBuildContext {
	char* buffer;
	char* position;
	uint16_t remaining_size;
	bool has_data;
	uint16_t sequence;
	
	CellBuildContext(char* buf, int size) 
		: buffer(buf), position(buf), remaining_size(size), has_data(false), sequence(0) {}
};

class UdpSocket;
class FifoQueue;
class Publisher;

class PubWorker {
	friend void* ProcsFunc(void* param);
public:
	// 构造与析构
	explicit PubWorker(int type);
	~PubWorker();

	// 基本接口
	void Init(uint32_t trans_id, int cell_size, int dst_port, char* dst_ip);
	void StartWork();
	void StopWork();
	void RecvPacket(char* packet, int len);
	void SetCellSize(int cell_size);
	void SetPublisher(Publisher* publisher);

	// 类型定义
	typedef std::map<int, int> DEVINZQMap;

	// 获取类型
	int pub_type() const { return pub_type_; }

private:
	// 主处理线程
	void ProcsMsg();

	void StopProcThrd();

	// 初始化相关
	void LoadLevelMap();
	void DisplayDeviceLevelMap();
	void InitializeThreadSync();

	// 消息处理相关
	void InitializeMessageCellBuffer(char* cell_buffer);
	void ResetMessageCellBuffer(char* cell_buffer, int head_len, uint16_t seq);
	void ProcessRemainingMessagePacket(char* cell_buffer, char*& data_pos, 
	                                 uint16_t& left_pack_size, int msg_type, bool& has_data_in_cell);
	void ProcessCompleteMessagePackets(char* cell_buffer, int msg_type, bool& has_data_in_cell);
	bool HasEnoughSpaceForNextPacket(char* cell_buffer, int head_len);
	bool GetNextPacket(char* data_buf, uint16_t& left_pack_size, char*& data_pos);
	void ProcessNewMessagePacket(char* cell_buffer, char*& data_pos, 
	                           uint16_t& left_pack_size, int msg_type, bool& has_data_in_cell);
	void TryAddMoreMessagePackets(char* cell_buffer, int msg_type, bool& has_data_in_cell);
	void SendMessageCell(char* cell_buffer, bool& has_data_in_cell);
	uint16_t CalculateRemainingSpace(char* cell_buffer);

	// Cell包构建与发送
	void CheckAndSndCell(char* cell_buffer, bool* has_data_in_cell);
	void SendCellWithNewProcess(char* cell_buffer, uint16_t devID);
	void SendCellWithLegacyProcess(char* cell_buffer, uint16_t devID);
	void SetSecurityLevel(char* cell_buffer, uint16_t devID);
	void SendLegacyCellWithControlHeader(char* cell_buffer);
	bool BuildDataCellFromPacket(char* data_buffer, uint16_t data_length, uint16_t dev_id, char* cell_buffer);
	uint16_t GetDeviceLabel(uint16_t dev_id);
	bool SendCellPacket(char* cell_buffer);

	// 消息/视频分包
	int PackCompleteMsgPack(int msg_type, char* cell_buffer, char*& cell_pos, bool* has_data_in_cell);
	bool PackMsgPacketHead(int pack_part_type, int msg_type, int len, uint16_t* frame_head);

	// 统计信息
	void UpdateStatistics();

	// 配置与状态
	int pub_type_;                // 发布类型
	uint32_t trans_id_;           // 事务ID
	int cell_size_;               // Cell包大小
	uint16_t security_level_;     // 安全级别
	bool work_over_;              // 工作状态

	// 网络相关
	UdpSocket* send_socket_;
	struct sockaddr_in target_addr_;

	// 数据队列
	FifoQueue* fifo_queue_;

	// 线程
	pthread_t proc_thread_;

	// 外部依赖
	Publisher* publisher_;

	// 设备安全级别映射
	DEVINZQMap devinZQ_map_;

	// 统计信息
	// 可扩展为结构体
};

#endif
