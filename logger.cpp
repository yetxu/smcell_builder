#include"logger.h"
#include"utils.h"
#include <stdarg.h>
using namespace std;

#define LOG_TO_CONSOLE

// 全局日志实例
Logger g_logger;

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

void Logger::LogToFile(LogLevel level, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char timestr[30];
    GetTime(timestr);
    
    const char* levelStr = GetLogLevelStr(level);
    
    fout_ << timestr << "[" << levelStr << "] " << buffer << endl << endl;

#ifdef LOG_TO_CONSOLE
    cout << timestr << "[" << levelStr << "] " << buffer << endl << endl;
#endif
}

const char* Logger::GetLogLevelStr(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}