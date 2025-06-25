#ifndef CELL_FACTORY_H_
#define CELL_FACTORY_H_
#include<stdint.h>
#include<stdio.h>

// 数据包最大尺寸
#define MAX_CELL_SIZE 1000
#define MAX_DATA_LEN 1000
#define CONTROL_HEADER_LEN 32
#define SECURITY_HEAD_LEN 11
#define SECURITY_LEV_POS 9
#define CHECK_SUM_SIZE 2
#define FRAME_HEAD_SIZE 2
#define STOP_TAG_SIZE 2

// 组包超时时间（毫秒）
#define PACK_TIMEOUT_MS 10
#define NETWORK_TAG_OFFSET 20  // 组网关系标签在cell_buffer中的偏移量

// 组包策略阈值
#define SMALL_DATA_THRESHOLD 128    // 小数据包阈值（字节）
#define MEDIUM_DATA_THRESHOLD 512   // 中等数据包阈值（字节）
#define MAX_MEDIUM_PART_SIZE 256    // 中等数据包分片大小
#define MAX_LARGE_PART_SIZE 512     // 大数据包分片大小

// 消息类型
#define REGISTER_RESPONSE_TYPE 0x01000001    // 注册响应帧类型
#define NETWORK_RELATION_CHANGE_TYPE 0x01000002  // 组网关系变化信令类型

// 数据接收标识
enum DataSourceType {
    NETWORK_DATA = 0x00,    // 网口数据
    SERIAL_DATA = 0x01,     // 串口数据
    LQ_DATA = 0x02,         // [lq]数据
    REGISTER_RESPONSE = 0x03, // 注册响应帧
    NETWORK_RELATION_CHANGE = 0x04 // 组网关系变化信令
};

// 组包策略
enum PackStrategy {
    CONTAINER_STRATEGY_1 = 0x01,  // 集装箱式策略1
    CONTAINER_STRATEGY_2 = 0x02,  // 集装箱式策略2
    CONTAINER_STRATEGY_3 = 0x03,  // 集装箱式策略3
    CONTAINER_STRATEGY_4 = 0x04,  // 集装箱式策略4
    NON_CONTAINER = 0x05,         // 非集装箱式
    FRAGMENT_FRAME = 0x06,        // 拆包分片
    PACK_STRATEGY_SMALL = 0x07,   // 小数据包策略
    PACK_STRATEGY_MEDIUM = 0x08,  // 中等数据包策略
    PACK_STRATEGY_LARGE = 0x09    // 大数据包策略
};

// 帧类型
enum FrameType {
    FRAME_HEAD = 0x01,      // 头包
    FRAME_MIDDLE = 0x02,    // 中间包
    FRAME_TAIL = 0x03       // 尾包
};

enum ServiceType{
    FILE_SERVICE = 0x01,
    VIDEO_SERVICE = 0x02,
    IPSTREAM_SERVICE = 0x03,
    MSG_SERVICE = 0x04,
    RADAR_SERVICE = 0x08
};

enum SmType{
    SM_TYPE_PROGRAME = 1,
    SM_TYPE_MESSAGE = 2
};

enum SmFlag{
    COMPLETE_MSG = 0x00,
    MSG_HEAD = 0x01,
    MSG_BODY = 0x02,
    MSG_TAIL = 0x03
};

enum VideoCellType{
    CELL_TYPE_UNKNOWN = 0,
    CELL_TYPE_HEAD = 1,
    CELL_TYPE_HEAD_TAIL_ONLY = 2,       //a complete muti-media pack
    CELL_TYPE_HEAD_TAIL_ANTH = 4,       //a complete multi-media pack and another one's part
    CELL_TYPE_HEAD_TAIL_ANTH_TAIL = 8,  //two complete multi-media packs

	CELL_TYPE_BODY = 16,                //body of a multi-media pack

    CELL_TYPE_TAIL_ONLY = 32,           //tail part of a multi-media pack

    CELL_TYPE_TAIL_ANTH = 64,           //tail part of a multi-media pack and another pack's partial

    CELL_TYPE_TAIL_ANTH_TAIL = 128      //tail part of a multi-media pack and another complete pack

};

//when pack or unpack cell,  use this pointer of MsgPacket or VideoPacket point
//to a positon in buffer, we can get or set the packet propertities automatically  EVEN 
//when considering the byte-alignment 
struct MsgPacket{

    uint16_t frame_head; //2 Bytes len frame head, bits 15-13 indicates the message type, 
                         //bits 12-11 indicates the block head flag, 00 is a complete block,  01 head part of a msg packet, 10 body, 11 tail
                         //bits 10-0 indicates the length of the following packet data
    char packet_data[MAX_DATA_LEN];
};

struct VideoPacket{
    uint8_t stream_format;
    uint8_t cell_type;
    uint16_t first_pack_tail;
    uint16_t body_length;
    char packet_data[MAX_DATA_LEN];
};


// 组网关系标签（2字节按位编码，区分四级站点归属）
struct NetworkTag {
    uint16_t level1 : 4;  // 第一级站点标识
    uint16_t level2 : 4;  // 第二级站点标识
    uint16_t level3 : 4;  // 第三级站点标识
    uint16_t level4 : 4;  // 第四级站点标识

    NetworkTag() : level1(0), level2(0), level3(0), level4(0) {}
};

// Cell包广播格式头部
struct CellBroadcastHeader {
    uint8_t security_control[11];  // 安全控制头
    uint8_t service_type;          // 业务类型 (0x08)
    uint32_t service_id;           // 业务ID
    uint8_t fixed_byte;            // 固定字节 (0x00)
    uint8_t business_type;         // 业务类型
    uint16_t flag_info;            // 标志信息
    
    CellBroadcastHeader() {
        memset(security_control, 0, 11);
        service_type = RADAR_SERVICE;
        fixed_byte = 0x00;
    }
};

// 注册响应帧结构
#pragma pack(push, 1)
struct RegisterResponseFrame {
    uint32_t msg_type;      // 消息类型 (REGISTER_RESPONSE_TYPE)
    uint32_t station_ip;    // 信息站IP
    uint8_t role;           // 站点角色 (1: 第四级站点, 2: 最高级站点, 0: 其他)
    uint8_t level1_id;      // 第一级站点标识
    uint8_t level2_id;      // 第二级站点标识
    uint8_t level3_id;      // 第三级站点标识
    uint8_t level4_id;      // 第四级站点标识
    uint8_t reserved[3];    // 保留字节，用于字节对齐
};

// 站点关系信息结构
struct StationRelationInfo {
    uint32_t station_ip;    // 信息站IP
    uint8_t role;           // 站点角色
    uint8_t level1_id;      // 第一级站点标识
    uint8_t level2_id;      // 第二级站点标识
    uint8_t level3_id;      // 第三级站点标识
    uint8_t level4_id;      // 第四级站点标识
    uint8_t reserved[3];    // 保留字节，用于字节对齐
};

// 组网关系变化信令结构
struct NetworkRelationChangeFrame {
    uint32_t msg_type;          // 消息类型 (NETWORK_RELATION_CHANGE_TYPE)
    uint16_t station_count;     // 站点数量
    uint16_t reserved;          // 保留字节，用于字节对齐
    StationRelationInfo stations[0];  // 变长数组，存储站点关系信息
};
#pragma pack(pop)

struct Cell {
    uint32_t cell_id;
    uint32_t unicast_ip;           // unicast ip, 4 bytes
    int data_len;                  // data length of cell
    int body_start_pos;            // cell body's start position
    DataSourceType source_type;    // 数据来源类型
    PackStrategy pack_strategy;    // 组包策略
    NetworkTag network_tag;        // 组网关系标签
    uint64_t timestamp;           // 时间戳，用于超时控制
    
    char cell_data[MAX_CELL_SIZE]; // 存储cell头部和数据体

    Cell() : unicast_ip(0), data_len(0), body_start_pos(0), 
             source_type(NETWORK_DATA), pack_strategy(CONTAINER_STRATEGY_1),
             timestamp(0) {
    }

    ~Cell() {
    }

    // 检查是否超时
    bool isTimeout() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t current = now.tv_sec * 1000 + now.tv_nsec / 1000000;
        return (current - timestamp) > PACK_TIMEOUT_MS;
    }
};


// 日志宏定义
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

#endif