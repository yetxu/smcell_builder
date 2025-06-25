#ifndef SIGNAL_DEFINE_H_
#define SIGNAL_DEFINE_H_
#include<stdint.h>


enum CippPacketType{
  PKT_TYPE_LOGIN = 0x00,
  PKT_TYPE_CONTROL = 0x01,
  PKT_TYPE_DATA = 0x02,

  PKT_TYPE_LOGIN_REPLY = 0x80,
  PKT_TYPE_CONTROL_REPLY = 0x81,
  PKT_TYPE_DATA_REPLY = 0x82,
};

enum CippLoginResult{
  LOGIN_OK = 0x00,
  LOGIN_FAILED = 0x81,
  LOGIN_INVALID_VAR = 0x82,
  LOGIN_RETRY = 0x83,
};

enum ControlType{
  CONTROL_CREATE = 0x00,
  CONTROL_UPDATE = 0x01,
  CONTROL_CANCEL = 0x02,
  CONTROL_QUERY = 0x03,
  CONTROL_TRANSACTION_INFORM = 0x10
};

enum TransactionType{
  TRANSACTION_FILE = 0x01,
  TRANSACTION_VIDEO = 0x02,
  TRANSACTION_SMS = 0x04,
};


#pragma pack(push)
#pragma pack(1)

struct PublishHeader{
    uint32_t seq;
    uint8_t src_role;
    uint8_t control_type;
    char data[];
};

struct Publish2AccessHeader{
    uint32_t seq;
    uint8_t control_type;
    uint8_t result;
    char data[];
};


struct CippHeader{
    uint8_t ver;
    uint8_t type;
    uint32_t len;
    uint8_t data[];
};

struct CippLogin{
  uint8_t id;
  char passwd[];
};

struct CippLoginReply{
  uint8_t id;
  uint8_t result;
};


struct CippCreate{
  uint32_t seq;
  uint8_t action;
  uint8_t id;
  uint8_t transaction_type;
  uint8_t wave_num;
  uint8_t mode; 
  uint32_t provider_id;
  uint16_t security_level;
  char data[];
};

struct CippQosparmeters{
  uint8_t qos_level;
  //可靠性，0表示不可靠，不进行喷泉编码；1表示可靠，需要进行喷泉编码
  uint8_t fidelity;
  uint32_t average_bandwidth;
  uint32_t max_bandwidth;
  uint64_t data_len;
  uint32_t max_delay;
  uint32_t max_delay_variation;
  uint32_t start_time;
  uint32_t start_valid_span;
  uint32_t duration;
  uint32_t end_time;
};

struct CippCreateReply{
  uint32_t seq;
  uint8_t action;
  uint8_t id;
  uint8_t result;
  uint32_t transaction_id;
  uint32_t access_ip;
  uint16_t access_port;
  uint8_t access_type;
};

struct CippData{
  uint8_t action;
  uint32_t transaction_id;
};

struct CippFinishReply{
  uint8_t action;
  uint32_t transaction_id;
  uint8_t status;
};

struct CippUpdate{
  uint32_t seq;
  uint8_t action;
  uint8_t id;
  uint32_t transaction_id;
  uint8_t transaction_type;
  uint8_t wave_num;
  uint8_t mode; 
  uint32_t provider_id;
  uint16_t security_level;
  char data[];
};

struct CippCancelQuery{
    uint32_t seq;
    uint8_t action;
    uint8_t id;
    uint32_t transaction_num;
    uint8_t data[];
};

#pragma pack(pop)
#endif
