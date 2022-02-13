## 如何运行

首先确保已经安装了MYSQL

```sql
//创建数据库
create database webdata;

//创建用户数据表
USE webdata;
CREATE TABLE user(username char(50) NULL,passwd char(50) NULL)ENGINE=InnoDB;
//添加数据
INSERT INTO user(username,passwd) VALUES('name','passwd');
```

修改`src/main.cpp`中的变量初始化数据库信息

```
//数据库登录名,密码,库名
string user = "root";
string passwd = "passwd";
string databasename = "webdata";
```

build并启动server

```
mkdir build
cd build
cmake ...
make
./server
```

在浏览器端键入，看到如下界面说明运行成功

```
http://localhost:8000/
```

![image-20220213145027176](root/jiemian.png)

## 项目介绍

本项目是我个人阅读游双大佬的《Linux高性能服务器编程》后，并参考其他的WebServer项目后实现的，一些模块包括有限状态机、Reactor模式、Proactor模式等等与书中基本一致。项目的主要内容如下所示

- 基于**线程池**、**Epoll复用(ET和LT均实现)+非阻塞IO**、**Proactor和Reactor事件处理模型**（二者选一）实现服务器的并发模型
- 使用**状态机**解析HTTP请求，可处理**GET**和**POST**请求，基于**Mysql**在Web端可实现用户注册、登陆，浏览图片、视频等功能
- 基于C++ Set数据结构实现**小根堆定时器容器**，处理超时非活动链接
- 实现简单的**异步日志**和**同步日志**系统
- **Proactor模式下**，主线程读取数据并封装，插入到请求队列中，由工作线程处理客户请求；**Reactor模式下**主线程只负责监听客户连接并下发socket到工作线程，由工作线程完成数据读取和处理。可以根据场景的不同选择不同的事件处理模式。
- 通过webbench进行压测，服务器可以实现**上万的并发连接**

项目的整体架构图如下所示(以Proactor模式为例)

![webserver.jpg](https://github.com/Yan-Xiaodi/Image_respority/blob/master/img/webserver.jpg?raw=true)



## 压测结果

**硬件平台：**

内存：8G

cpu：i5-7300HQ

系统：Ubuntu18.04

这个是在LT模式下的测试结果，ET结果与之类似，可以看出服务器具有较好的并发性能，可以同时处理上万的连接及数据传输

![2022-02-04 11-04-33 的屏幕截图.png](https://github.com/Yan-Xiaodi/Image_respority/blob/master/img/2022-02-04%2011-04-33%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png?raw=true)



## 访问服务器资源示例



<video id="video" controls="" preload="none" >
      <source id="mp4" src="./root/wangye.mp4" type="video/mp4">
</videos>

