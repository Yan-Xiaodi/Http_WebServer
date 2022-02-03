#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "locker.h"
#include <deque>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
using namespace std;

template <class T>
class block_queue
{
  public:
    block_queue(int max_size = 1000)
    {
        if (max_size <= 0) {
            exit(-1);
        }
        capacity_ = max_size;
    }

    void clear()
    {
        mutex_.lock();
        queue_.clear();
        queue_.mutex_.unlock();
    }

    ~block_queue()
    {
        mutex_.lock();
        clear();
        mutex_.unlock();
    }
    //判断队列是否满了
    bool full()
    {
        mutex_.lock();
        int size = queue_.size();
        mutex_.unlock();
        return size >= capacity_;
    }
    //判断队列是否为空
    bool empty()
    {
        mutex_.lock();
        bool isempty = queue_.empty();
        mutex_.unlock();
        return isempty;
    }
    //返回队首元素
    bool front(T& value)
    {
        mutex_.lock();
        if (queue_.empty()) {
            mutex_.unlock();
            return false;
        }
        value = queue_.front();
        mutex_.unlock();
        return true;
    }
    //返回队尾元素
    bool back(T& value)
    {
        mutex_.lock();
        if (queue_.empty()) {
            mutex_.unlock();
            return false;
        }
        value = queue_.back();
        mutex_.unlock();
        return true;
    }

    int size()
    {
        mutex_.lock();
        int tmp = queue_.size();
        mutex_.unlock();
        return tmp;
    }

    int max_size()
    {
        mutex_.lock();
        int tmp = capacity_;
        mutex_.unlock();
        return tmp;
    }
    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T& item)
    {

        mutex_.lock();
        if (queue_.size() >= capacity_) {

            cond_t.broadcast();
            mutex_.unlock();
            return false;
        }

        queue_.push_back(item);
        // broadcast的作用是唤醒所有睡眠中的线程，让它们从阻塞中退出
        cond_t.broadcast();
        mutex_.unlock();
        return true;
    }
    // pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T& item)
    {

        mutex_.lock();
        while (queue_.empty()) {
            //队列为空，就应该进入阻塞状态，等待cond，被唤醒后才可以进行下一步
            if (!cond_t.wait(mutex_.get())) {
                mutex_.unlock();
                /*为什么这里应该返回false，因为当这个线程占用队列时，为空，等待条件变量cond唤醒后
                队列中已有元素，但是对于本线程而言，进入时为空仍应该返回false*/
                return false;
            }
        }
        item = queue_.front();
        queue_.pop_front();
        mutex_.unlock();
        return true;
    }

    //增加了超时处理
    bool pop(T& item, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        mutex_.lock();
        if (queue_.empty()) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!cond_t.timewait(mutex_.get(), t)) {
                mutex_.unlock();
                return false;
            }
        }

        // if (queue_.empty()) {
        //     mutex_.unlock();
        //     return false;
        // }
        item = queue_.front();
        queue_.pop_front();
        mutex_.unlock();
        return true;
    }

  private:
    locker mutex_;
    cond cond_t;

    std::deque<T> queue_;
    int capacity_;
};

#endif
