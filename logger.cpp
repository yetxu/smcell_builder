#include"logger.h"
#include"utils.h"
using namespace std;

#define LOG_TO_CONSOLE

Logger::Logger(){

}

Logger::~Logger(){
    fout_.close();
}

void Logger::Init(const char* logfile){
    fout_.open(logfile, ios::out);
    assert(fout_.is_open());
}

void Logger::LogToFile(char* msg){
    char timestr[30];
    GetTime(timestr);
    fout_ << timestr;
    fout_ << msg << endl << endl;

#ifdef LOG_TO_CONSOLE
    cout << timestr;
    cout << msg << endl << endl;
#endif
}

void Logger::LogToFile(const char* msg){
    char timestr[30];
    GetTime(timestr);
    fout_ << timestr;
    fout_ << msg << endl << endl;

#ifdef LOG_TO_CONSOLE
    cout << timestr;
    cout << msg << endl << endl;
#endif

}
