#ifndef LOGGER_H_
#define LOGGER_H_
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <string>

// 日志级别枚举
enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};
class Logger{
public:
    Logger();
    ~Logger();

    void LogToFile(char* msg);
    void LogToFile(const char* msg);
    void LogToFile(LogLevel level, const char* format, ...);
    void Init(const char* logfile);
private:
    const char* GetLogLevelStr(LogLevel level);
    std::ofstream fout_;
};  

// 全局日志实例
extern Logger g_logger;

// 日志宏定义
#define LOG_INFO(format, ...) \
    g_logger.LogToFile(LOG_LEVEL_INFO, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    g_logger.LogToFile(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#define LOG_DEBUG(format, ...) \
    g_logger.LogToFile(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) \
    g_logger.LogToFile(LOG_LEVEL_WARN, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) \
    g_logger.LogToFile(LOG_LEVEL_FATAL, format, ##__VA_ARGS__)

#endif