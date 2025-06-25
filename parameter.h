#ifndef PARAMETER_H_
#define PARAMETER_H_

#define DISPLAY_PARAMETER
#define CONFIG_FILE "./config.json"

enum class NetworkMode { UNICAST, MULTICAST, BROADCAST };

class Parameter{

public:
    Parameter();
    ~Parameter();

    void LoadConfig();
    void SaveConfig();

public:
    enum{
        MAX_IP_SIZE = 16
    };
    NetworkMode net_mode;
    int cell_length;

    int radar_recv_port;

    int inner_radar_port;


    char dst_ip[MAX_IP_SIZE];  //the destation ipaddress of cell from pubserver

    int dst_port;              //the destination port of cell from pubserver

    int stream_server_num;     //max number of stream server
};

#endif
