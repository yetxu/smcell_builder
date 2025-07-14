#ifndef PARAMETER_H_
#define PARAMETER_H_

#define DISPLAY_PARAMETER
#define CONFIG_FILE "./config.json"

class Parameter{

public:
    Parameter();
    ~Parameter();

    void LoadConfig();
    void SaveConfig();

public:
    enum{
        MAX_IP_SIZE = 16,
        MAX_DB_NAME_SIZE = 64,
        MAX_DB_USER_SIZE = 32,
        MAX_DB_PASS_SIZE = 32
    };

    int cell_length;

    int radar_recv_port;

    int inner_radar_port;


    char dst_ip[MAX_IP_SIZE];  //the destation ipaddress of cell from pubserver

    int dst_port;              //the destination port of cell from pubserver

    int stream_server_num;     //max number of stream server

    int transaction_id;        //transaction id

    char db_host[MAX_IP_SIZE];     // Database host address
    int db_port;                   // Database port
    char db_name[MAX_DB_NAME_SIZE];// Database name
    char db_user[MAX_DB_USER_SIZE];// Database user
    char db_pass[MAX_DB_PASS_SIZE];// Database password
};

#endif
