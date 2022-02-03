#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "locker.h"
#include "sql_connection_pool.h"
#include <cstdio>
#include <exception>
#include <list>
#include <pthread.h>

template <typename T>
class threadpool
{
  public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int actor_model, connection_pool* connPool, int thread_number = 8,
               int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);
    bool append_p(T* request);

  private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void* worker(void* arg);
    void run();

  private:
    int thread_number_;         //线程池中的线程数
    int max_requests_;          //请求队列中允许的最大请求数
    pthread_t* threads_;        //描述线程池的数组，其大小为thread_number_
    std::list<T*> workqueue_;   //请求队列
    locker queuelocker_;        //保护请求队列的互斥锁
    sem queuestat_;             //是否有任务需要处理
    connection_pool* connPool_; //数据库
    int actor_model_;           //模型切换
};
template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number,
                          int max_requests)
    : actor_model_(actor_model), thread_number_(thread_number), max_requests_(max_requests),
      threads_(NULL), connPool_(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    threads_ = new pthread_t[thread_number_];
    if (!threads_)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i) {
        if (pthread_create(threads_ + i, NULL, worker, this) != 0) {
            delete[] threads_;
            throw std::exception();
        }
        if (pthread_detach(threads_[i])) {
            delete[] threads_;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] threads_;
}
template <typename T>
bool threadpool<T>::append(T* request, int state)
{
    queuelocker_.lock();
    if (workqueue_.size() >= max_requests_) {
        queuelocker_.unlock();
        return false;
    }
    request->m_state = state;
    workqueue_.push_back(request);
    queuelocker_.unlock();
    queuestat_.post();
    return true;
}
template <typename T>
bool threadpool<T>::append_p(T* request)
{
    queuelocker_.lock();
    if (workqueue_.size() >= max_requests_) {
        queuelocker_.unlock();
        return false;
    }
    workqueue_.push_back(request);
    queuelocker_.unlock();
    queuestat_.post();
    return true;
}
template <typename T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}
template <typename T>
void threadpool<T>::run()
{
    while (true) {
        queuestat_.wait();
        queuelocker_.lock();
        if (workqueue_.empty()) {
            queuelocker_.unlock();
            continue;
        }
        T* request = workqueue_.front();
        workqueue_.pop_front();
        queuelocker_.unlock();
        if (!request)
            continue;
        if (1 == actor_model_) {
            // 1表示reactor模型
            // m_state==0表示读取数据
            if (0 == request->m_state) {
                if (request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, connPool_);
                    request->process();
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {
                //表示写数据
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else {
            // proactor模式，读写都由主线程实现
            connectionRAII mysqlcon(&request->mysql, connPool_);
            request->process();
        }
    }
}
#endif
