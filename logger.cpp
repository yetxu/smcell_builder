#include "logger.h"
#include "utils.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>

#define LOG_TO_CONSOLE
#define MAX_LOG_BUFFER_SIZE 4096
#define MAX_TIMESTAMP_SIZE 64

using namespace std;

Logger::Logger() : current_level_(LOG_LEVEL_INFO), initialized_(false), log_count_(0) {
    pthread_mutex_init(&log_mutex_, NULL);
    gettimeofday(&start_time_, NULL);
}

Logger::~Logger() {
    if (initialized_) {
        fout_.close();
    }
    pthread_mutex_destroy(&log_mutex_);
}

void Logger::Init(const char* logfile, LogLevel level) {
    pthread_mutex_lock(&log_mutex_);
    
    current_level_ = level;
    fout_.open(logfile, ios::out | ios::app);
    assert(fout_.is_open());
    initialized_ = true;
    
    // 写入启动日志
    char timestamp[MAX_TIMESTAMP_SIZE];
    FormatTimestamp(timestamp, MAX_TIMESTAMP_SIZE);
    fout_ << timestamp << " [INFO ] Logger initialized with level: " << GetLevelString(level) << endl;
    
    pthread_mutex_unlock(&log_mutex_);
}

void Logger::SetLogLevel(LogLevel level) {
    pthread_mutex_lock(&log_mutex_);
    current_level_ = level;
    pthread_mutex_unlock(&log_mutex_);
}

void Logger::LogToFile(const char* msg) {
    LogToFile(LOG_LEVEL_INFO, "%s", msg);
}

void Logger::LogToFile(char* msg) {
    LogToFile(LOG_LEVEL_INFO, "%s", msg);
}

void Logger::LogToFile(LogLevel level, const char* format, ...) {
    if (!IsLevelEnabled(level)) return;
    
    va_list args;
    va_start(args, format);
    WriteLog(level, NULL, 0, format, args);
    va_end(args);
}

// 各级别日志函数实现
void Logger::LogTrace(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_TRACE)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_TRACE, NULL, 0, format, args);
    va_end(args);
}

void Logger::LogDebug(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_DEBUG)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_DEBUG, NULL, 0, format, args);
    va_end(args);
}

void Logger::LogInfo(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_INFO)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_INFO, NULL, 0, format, args);
    va_end(args);
}

void Logger::LogWarn(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_WARN)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_WARN, NULL, 0, format, args);
    va_end(args);
}

void Logger::LogError(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_ERROR)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_ERROR, NULL, 0, format, args);
    va_end(args);
}

void Logger::LogFatal(const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_FATAL)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_FATAL, NULL, 0, format, args);
    va_end(args);
}

// 带文件行号的日志函数实现
void Logger::LogTrace(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_TRACE)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_TRACE, file, line, format, args);
    va_end(args);
}

void Logger::LogDebug(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_DEBUG)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_DEBUG, file, line, format, args);
    va_end(args);
}

void Logger::LogInfo(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_INFO)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_INFO, file, line, format, args);
    va_end(args);
}

void Logger::LogWarn(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_WARN)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_WARN, file, line, format, args);
    va_end(args);
}

void Logger::LogError(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_ERROR)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_ERROR, file, line, format, args);
    va_end(args);
}

void Logger::LogFatal(const char* file, int line, const char* format, ...) {
    if (!IsLevelEnabled(LOG_LEVEL_FATAL)) return;
    va_list args;
    va_start(args, format);
    WriteLog(LOG_LEVEL_FATAL, file, line, format, args);
    va_end(args);
}

// 性能日志函数
void Logger::LogPerformance(const char* operation, double duration_ms) {
    if (!IsLevelEnabled(LOG_LEVEL_INFO)) return;
    
    pthread_mutex_lock(&log_mutex_);
    
    char timestamp[MAX_TIMESTAMP_SIZE];
    FormatTimestamp(timestamp, MAX_TIMESTAMP_SIZE);
    
    stringstream ss;
    ss << timestamp << " [PERF] " << operation << " took " << fixed << setprecision(2) << duration_ms << " ms";
    
    fout_ << ss.str() << endl;
    
#ifdef LOG_TO_CONSOLE
    cout << ss.str() << endl;
#endif
    
    pthread_mutex_unlock(&log_mutex_);
}

// 网络日志函数
void Logger::LogNetwork(const char* operation, const char* ip, int port, int bytes) {
    if (!IsLevelEnabled(LOG_LEVEL_INFO)) return;
    
    pthread_mutex_lock(&log_mutex_);
    
    char timestamp[MAX_TIMESTAMP_SIZE];
    FormatTimestamp(timestamp, MAX_TIMESTAMP_SIZE);
    
    stringstream ss;
    ss << timestamp << " [NET ] " << operation << " " << ip << ":" << port << " (" << bytes << " bytes)";
    
    fout_ << ss.str() << endl;
    
#ifdef LOG_TO_CONSOLE
    cout << ss.str() << endl;
#endif
    
    pthread_mutex_unlock(&log_mutex_);
}

// 数据库日志函数
void Logger::LogDatabase(const char* operation, const char* query, bool success) {
    if (!IsLevelEnabled(LOG_LEVEL_INFO)) return;
    
    pthread_mutex_lock(&log_mutex_);
    
    char timestamp[MAX_TIMESTAMP_SIZE];
    FormatTimestamp(timestamp, MAX_TIMESTAMP_SIZE);
    
    stringstream ss;
    ss << timestamp << " [DB  ] " << operation << " " << (success ? "SUCCESS" : "FAILED") << " - " << query;
    
    fout_ << ss.str() << endl;
    
#ifdef LOG_TO_CONSOLE
    cout << ss.str() << endl;
#endif
    
    pthread_mutex_unlock(&log_mutex_);
}

// 私有方法实现
void Logger::FormatTimestamp(char* buffer, size_t size) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    struct tm* timeinfo = localtime(&now.tv_sec);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday,
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec,
             now.tv_usec / 1000);
}

const char* Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_TRACE: return LOG_LEVEL_STR_TRACE;
        case LOG_LEVEL_DEBUG: return LOG_LEVEL_STR_DEBUG;
        case LOG_LEVEL_INFO:  return LOG_LEVEL_STR_INFO;
        case LOG_LEVEL_WARN:  return LOG_LEVEL_STR_WARN;
        case LOG_LEVEL_ERROR: return LOG_LEVEL_STR_ERROR;
        case LOG_LEVEL_FATAL: return LOG_LEVEL_STR_FATAL;
        default: return "UNKNOWN";
    }
}

void Logger::WriteLog(LogLevel level, const char* file, int line, const char* format, va_list args) {
    pthread_mutex_lock(&log_mutex_);
    
    char timestamp[MAX_TIMESTAMP_SIZE];
    FormatTimestamp(timestamp, MAX_TIMESTAMP_SIZE);
    
    char message_buffer[MAX_LOG_BUFFER_SIZE];
    vsnprintf(message_buffer, MAX_LOG_BUFFER_SIZE, format, args);
    
    stringstream ss;
    ss << timestamp << " [" << GetLevelString(level) << "] ";
    
    if (file && line > 0) {
        ss << "[" << GetFileName(file) << ":" << line << "] ";
    }
    
    ss << message_buffer;
    
    fout_ << ss.str() << endl;
    fout_.flush(); // 确保立即写入文件
    
#ifdef LOG_TO_CONSOLE
    cout << ss.str() << endl;
#endif
    
    log_count_++;
    
    pthread_mutex_unlock(&log_mutex_);
}

const char* Logger::GetFileName(const char* file_path) {
    const char* filename = strrchr(file_path, '/');
    if (filename) {
        return filename + 1;
    }
    
    filename = strrchr(file_path, '\\');
    if (filename) {
        return filename + 1;
    }
    
    return file_path;
}

// 全局日志对象定义
Logger g_logger;
