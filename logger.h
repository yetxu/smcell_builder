#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>
#include <string>
#include <cstdarg>
#include <pthread.h>
#include <sys/time.h>

// 日志级别枚举
enum LogLevel {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5
};

// 日志级别字符串
#define LOG_LEVEL_STR_TRACE "TRACE"
#define LOG_LEVEL_STR_DEBUG "DEBUG"
#define LOG_LEVEL_STR_INFO  "INFO "
#define LOG_LEVEL_STR_WARN  "WARN "
#define LOG_LEVEL_STR_ERROR "ERROR"
#define LOG_LEVEL_STR_FATAL "FATAL"

class Logger {
public:
    Logger();
    ~Logger();

    // 初始化日志系统
    void Init(const char* logfile, LogLevel level = LOG_LEVEL_INFO);
    
    // 设置日志级别
    void SetLogLevel(LogLevel level);
    
    // 基础日志函数
    void LogToFile(const char* msg);
    void LogToFile(char* msg);
    
    // 格式化日志函数
    void LogToFile(LogLevel level, const char* format, ...);
    
    // 各级别日志函数
    void LogTrace(const char* format, ...);
    void LogDebug(const char* format, ...);
    void LogInfo(const char* format, ...);
    void LogWarn(const char* format, ...);
    void LogError(const char* format, ...);
    void LogFatal(const char* format, ...);
    
    // 带文件行号的日志函数
    void LogTrace(const char* file, int line, const char* format, ...);
    void LogDebug(const char* file, int line, const char* format, ...);
    void LogInfo(const char* file, int line, const char* format, ...);
    void LogWarn(const char* file, int line, const char* format, ...);
    void LogError(const char* file, int line, const char* format, ...);
    void LogFatal(const char* file, int line, const char* format, ...);
    
    // 性能日志函数
    void LogPerformance(const char* operation, double duration_ms);
    
    // 网络日志函数
    void LogNetwork(const char* operation, const char* ip, int port, int bytes);
    
    // 数据库日志函数
    void LogDatabase(const char* operation, const char* query, bool success);
    
    // 获取当前日志级别
    LogLevel GetLogLevel() const { return current_level_; }
    
    // 检查是否启用某个日志级别
    bool IsLevelEnabled(LogLevel level) const { return level >= current_level_; }

private:
    // 格式化时间戳
    void FormatTimestamp(char* buffer, size_t size);
    
    // 获取日志级别字符串
    const char* GetLevelString(LogLevel level);
    
    // 写入日志（线程安全）
    void WriteLog(LogLevel level, const char* file, int line, const char* format, va_list args);
    
    // 获取文件名（去掉路径）
    const char* GetFileName(const char* file_path);

private:
    std::ofstream fout_;
    LogLevel current_level_;
    pthread_mutex_t log_mutex_;
    bool initialized_;
    
    // 性能统计
    struct timeval start_time_;
    unsigned long log_count_;
};

// 全局日志宏定义
#define LOG_TRACE(format, ...) g_logger.LogTrace(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) g_logger.LogDebug(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  g_logger.LogInfo(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  g_logger.LogWarn(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) g_logger.LogError(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) g_logger.LogFatal(__FILE__, __LINE__, format, ##__VA_ARGS__)

// 性能日志宏
#define LOG_PERFORMANCE(operation, duration_ms) g_logger.LogPerformance(operation, duration_ms)

// 网络日志宏
#define LOG_NETWORK(operation, ip, port, bytes) g_logger.LogNetwork(operation, ip, port, bytes)

// 数据库日志宏
#define LOG_DATABASE(operation, query, success) g_logger.LogDatabase(operation, query, success)

// 条件日志宏（只在DEBUG模式下输出）
#ifdef DEBUG
#define LOG_DEBUG_COND(format, ...) LOG_DEBUG(format, ##__VA_ARGS__)
#else
#define LOG_DEBUG_COND(format, ...)
#endif

#endif // LOGGER_H_
