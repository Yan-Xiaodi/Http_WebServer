#ifndef LST_TIMER
#define LST_TIMER

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <set>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"
#include <time.h>

class util_timer;

struct client_data {
    sockaddr_in address;
    int sockfd;
    util_timer* timer;
};

class util_timer
{
  public:
    util_timer() : prev(NULL), next(NULL) {}

  public:
    time_t expire;

    void (*cb_func)(client_data*);
    client_data* user_data;
    util_timer* prev;
    util_timer* next;
};

// timer_set的排序比较方法
struct cmp {
    bool operator()(const util_timer* a, const util_timer* b) { return a->expire < b->expire; }
};

class timer_heap
{
  public:
    timer_heap();
    ~timer_heap();

    void add_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();

  private:
    void add_timer(util_timer* timer, util_timer* lst_head);
    //使用set作为定时器的容器，set的底层为红黑树，插入删除访问元素都具有对数时间复杂度，在高并发情况下效率较高
    //同时set内部元素的更新比priority_queue更为方便
    std::set<util_timer*, cmp> timer_set;
};

class Utils
{
  public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char* info);

  public:
    static int* u_pipefd;
    timer_heap m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data* user_data);

#endif
