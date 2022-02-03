#include "sql_connection_pool.h"
#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

connection_pool::connection_pool()
{
    CurConn_ = 0;
    FreeConn_ = 0;
}

connection_pool* connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

//构造初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port,
                           int MaxConn, int close_log)
{
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DBName;
    m_close_log = close_log;

    for (int i = 0; i < MaxConn; i++) {
        MYSQL* con = NULL;
        MYSQL* ret = NULL;
        ret = mysql_init(ret);

        if (ret == NULL) {
            LOG_ERROR("MySQL Error");
            exit(1);
        } else {
            con = ret;
        }
        ret = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(),
                                 Port, NULL, 0);

        if (ret == NULL) {
            LOG_ERROR("MySQL Error\n");
            string err_info(mysql_error(con));
            err_info = (string("MySQL Error[error=") + std::to_string(mysql_errno(con)) +
                        string("]") + err_info);
            LOG_ERROR(err_info.c_str());
            exit(1);
        } else {
            con = ret;
        }
        connList.push_back(con);
        ++FreeConn_;
    }

    reserve = sem(FreeConn_);

    MaxConn_ = FreeConn_;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* connection_pool::GetConnection()
{
    MYSQL* con = NULL;

    if (0 == connList.size())
        return NULL;

    reserve.wait();

    lock.lock();

    con = connList.front();
    connList.pop_front();

    --FreeConn_;
    ++CurConn_;

    lock.unlock();
    return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL* con)
{
    if (NULL == con)
        return false;

    lock.lock();

    connList.push_back(con);
    ++FreeConn_;
    --CurConn_;

    lock.unlock();

    reserve.post();
    return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{

    lock.lock();
    if (connList.size() > 0) {
        list<MYSQL*>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it) {
            MYSQL* con = *it;
            mysql_close(con);
        }
        CurConn_ = 0;
        FreeConn_ = 0;
        connList.clear();
    }

    lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn() { return this->FreeConn_; }

connection_pool::~connection_pool() { DestroyPool(); }

connectionRAII::connectionRAII(MYSQL** SQL, connection_pool* connPool)
{
    *SQL = connPool->GetConnection();

    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() { poolRAII->ReleaseConnection(conRAII); }