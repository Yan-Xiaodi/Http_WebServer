#include "timerHeap.h"
#include "http_conn.h"

timer_heap::timer_heap() {}
timer_heap::~timer_heap()
{
    for (auto it = timer_set.begin(); it != timer_set.end(); ++it) {
        delete *it;
    }
    timer_set.clear();
}

void timer_heap::add_timer(util_timer* timer) { timer_set.insert(timer); }
void timer_heap::adjust_timer(util_timer* timer)
{
    //更新元素timer，首先将timer从集合中删除，然后重新插入
    util_timer* timer_new = timer;
    timer_set.erase(timer);
    timer_set.insert(timer_new);
}
void timer_heap::del_timer(util_timer* timer)
{
    timer_set.erase(timer);
    delete timer;
}
void timer_heap::tick()
{
    time_t cur = time(NULL);
    while (!timer_set.empty()) {
        auto it = timer_set.begin();
        if (cur < (*it)->expire) {
            break;
        }
        (*it)->cb_func((*it)->user_data); //处理超时的任务
        timer_set.erase(*it);
        delete *it;
    }
}

void timer_heap::add_timer(util_timer* timer, util_timer* lst_head) { timer_set.insert(timer); }

void Utils::init(int timeslot) { m_TIMESLOT = timeslot; }

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT); //按照m_TIMESLOT的时间出发SIGALRM信号
}

void Utils::show_error(int connfd, const char* info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int* Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data* user_data)
{
    //关闭超时连接
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    //连接数量-1
    http_conn::m_user_count--;
}
