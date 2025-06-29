#ifndef CELL_FACTORY_H_
#define CELL_FACTORY_H_
#include<stdint.h>

#define MAX_CELL_SIZE 1000
#define MAX_DATA_LEN 1000
#define CONTROL_HEADER_LEN 32
#define SECURITY_HEAD_LEN 11
#define SECURITY_LEV_POS 9
#define CHECK_SUM_SIZE 2
#define FRAME_HEAD_SIZE 2
#define STOP_TAG_SIZE 2

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

typedef std::map<std::string, std::string> NetworkRelationMap;

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


struct Cell{
    uint32_t cell_id;

    uint32_t unicast_ip;    //unicast ip, this is not necessary, 4 bytes len
    
    int data_len;           //data length of cell

    int body_start_pos;     //the cell body's start position

    char cell_data[MAX_CELL_SIZE]; 
    //pointer to buffer where store cell head and body,  in cell_data, we put all information of cell, ie. cell_id, unicast_ip, cel_type, etc

    Cell():unicast_ip(0), data_len(0), body_start_pos(0){
    };

    ~Cell(){
    }
};

#endif
