#ifndef LOGGER_H_
#define LOGGER_H_
#include<fstream>
class Logger{
public:
    Logger();
    ~Logger();

    void LogToFile(char* msg);
    void LogToFile(const char* msg);
    void Init(const char* logfile);
private:
    std::ofstream fout_;
};  
#endif
