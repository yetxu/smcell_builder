#include<fstream>
#include<iostream>
#include<string.h>
#include"parameter.h"
#include"json/reader.h"
#include"json/writer.h"

using namespace std;
using namespace json;

Parameter::Parameter(){
    
}

Parameter::~Parameter(){
//    SaveConfig();
}


void Parameter::LoadConfig(){
    Object root;
    //read config from file
    ifstream fin;
    fin.open(CONFIG_FILE);
    assert(fin.is_open());
    Reader::Read(root, fin);
    fin.close();

    //set config json object to parameters
    cell_length = Number(root["cell_length"]).Value();

    radar_recv_port = Number(root["radar_recv_port"]).Value();

    inner_radar_port = Number(root["inner_radar_port"]).Value();

    strcpy(dst_ip, String(root["dst_ip"]).Value().c_str());
    dst_port = Number(root["dst_port"]).Value();

    stream_server_num  = Number(root["stream_server_num"]).Value();

    transaction_id = Number(root["transaction_id"]).Value();

    if (root.Find("database") != root.End()) {
        const Object& db_config = root["database"];
        
        strcpy(db_host, String(db_config["host"]).Value().c_str());
        db_port = Number(db_config["port"]).Value();
        strcpy(db_name, String(db_config["name"]).Value().c_str());
        strcpy(db_user, String(db_config["user"]).Value().c_str());
        strcpy(db_pass, String(db_config["password"]).Value().c_str());
    }


#ifdef DISPLAY_PARAMETER
    cout.setf(ios::left);

    cout << setw(20)  << "cell_length " << cell_length << endl;
    cout << setw(20)  << "radar_recv_port " << radar_recv_port << endl;
    cout << setw(20)  << "radar_video_port " << inner_radar_port <<endl;
    cout << setw(20)  << "dst ip " << dst_ip << endl;
    cout << setw(20)  << "dst port " << dst_port << endl;

    cout << setw(20)  << "db host " << db_host << endl;
    cout << setw(20)  << "db port " << db_port << endl;
    cout << setw(20)  << "db name " << db_name << endl;
    cout << setw(20)  << "db user " << db_user << endl;
    cout << setw(20)  << "db pass " << db_pass << endl;
#endif
}

void Parameter::SaveConfig(){
    Object root;
    //set parameters to config json object
    root["cell_length"] = Number(cell_length);

    root["radar_recv_port"] = Number(radar_recv_port);

	root["inner_radar_port"] = Number(inner_radar_port);

    root["dst_ip"] = String(dst_ip);
    root["dst_port"] = Number(dst_port);
    root["stream_server_num"] = Number(stream_server_num);


    //write config to file
    ofstream fout;
    fout.open(CONFIG_FILE);
    assert(fout.is_open());
    Writer::Write(root, fout);
    fout.close();   
}
