#ifndef LOG_H
#define LOG_H

#include "block_queue.h"
#include <iostream>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>

using namespace std;

class Log
{
  public:
    // C++11以后,使用局部变量懒汉不用加锁
    static Log* get_instance()
    {
        static Log instance;
        return &instance;
    }

    static void* flush_log_thread(void* args) { Log::get_instance()->async_write_log(); }
    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char* file_name, int close_log, int log_buf_size = 8192,
              int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...);

    void flush(void);
    static int close_log_; //关闭日志

  private:
    Log();
    virtual ~Log();
    void* async_write_log()
    {
        string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (log_queue_->pop(single_log)) {
            mutex_.lock();
            fputs(single_log.c_str(), fp_);
            mutex_.unlock();
        }
    }

  private:
    char dir_name[128]; //路径名
    char log_name[128]; // log文件名
    int split_lines_;   //日志最大行数
    int logbuf_size_;   //日志缓冲区大小
    long long count_;   //日志行数记录
    int today_;         //因为按天分类,记录当前时间是那一天
    FILE* fp_;          //打开log的文件指针
    char* buf_;
    block_queue<string>* log_queue_; //阻塞队列
    bool is_async;                   //是否同步标志位
    locker mutex_;
};

#define LOG_DEBUG(format, ...)                                                                     \
    if (!Log::close_log_) {                                                                        \
        Log::get_instance()->write_log(0, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_INFO(format, ...)                                                                      \
    if (!Log::close_log_) {                                                                        \
        Log::get_instance()->write_log(1, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_WARN(format, ...)                                                                      \
    if (!Log::close_log_) {                                                                        \
        Log::get_instance()->write_log(2, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_ERROR(format, ...)                                                                     \
    if (!Log::close_log_) {                                                                        \
        Log::get_instance()->write_log(3, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }

#endif
