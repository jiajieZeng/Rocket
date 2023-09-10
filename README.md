# Rocket

rocket 是基于 C++11 开发的一款多线程的异步 RPC 框架，它旨在高效、简洁的同时，又保持至极高的性能。

rocket 同样是基于主从 Reactor 架构，底层采用 epoll 实现 IO 多路复用。应用层则基于 protobuf 自定义 rpc 通信协议，同时也将支持简单的 HTTP 协议。

整个框架开发大致已经完结。直接自己make是可以运行的，但是如果用脚手架生成的暂时buggy，由于开学没时间调试了。
TODO:
```
脚手架完善
添加协程
```

## 环境配置
### 环境搭建
* 开发环境：Linux服务器, 操作系统是Ubuntu18.04.4 TLS。使用GCC/G++, 目前使用C++11进行开发
* 开发工具：VsCode
### 目录层次结构
```
Rocket
  ├─bin         
  │  └─测试程序，可执行层序
  ├─conf        
  │  └─测试用的xml配置文件
  ├─lib         
  │  └─编译完成的静态库librocket.a
  ├─obj         
  │  └─所有编译主键文件,*.o
  ├─rocket      
  │  └─所有源代码
  └─testcases   
     └─测试代码
    
```
### 依赖库
#### protobuf
protobuf-3.19.4
安装过程：
```
cd ~

wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protobuf-cpp-3.19.4.tar.gz

tar -xzvf protobuf-cpp-3.19.4.tar.gz
```
指定安装路径：
```
cd protobuf-cpp-3.19.4

./configure -prefix=/usr/local

make -j4 

sudo make install
```
安装完成后，你可以找到头文件将位于 /usr/include/google 下，库文件将位于 /usr/lib 下。

#### tinyxml
项目中使用到了配置模块，采用了 xml 作为配置文件。因此需要安装 libtinyxml 解析 xml 文件。
```
wget https://udomain.dl.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip

unzip tinyxml_2_6_2.zip
```

需要修改tinyxml的`Makefile`文件：
* 将其中的`OUTPUT := xmltest`一行修改为：`OUTPUT := libtinyxml.a`
* 将`xmltest.cpp`从`SRCS：=tinyxml.cpp tinyxml-parser.cpp xmltest.cpp tinyxmlerror.cpp tinystr.cpp`中删除
* 注释掉`xmltest.o：tinyxml.h tinystr.h`。因为不需要将演示程序添加到动态库中。
* 将`${LD} -o 
{LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}` 修改为：`${AR} 
{LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}`


修改好之后：
```
make -j4
```

把`libtimyxml.a`复制到`/usr/lib/`
把`make`产生的`.h`文件，全部放入一个新的叫`tinyxml`的文件夹里面，移动到`/usr/include/`


### 日志开发
日志模块：
```
1. 日志级别
2. 打印到文件，支持日期明明，以及日志的滚动。
3. c 格式化风控
3. 线程安全
```

LogLevel:
```
Debug
Info
Error
```

LogEvent:
```
文件名，行号
MsgNo
进程号
Thread id
日期，以及时间。精确到 ms
自定义时间
```

日志格式
```
[level][%y-%m-%d %H:%M:%s.%ms]\t[pid:thread_id]\t[file_name:line][%msg]
```

Logger 日志器:
* 提供打印日志的方法 
* 设置日志输出的路径

### Reactor
Reactor，又可以称为 EventLoop，它的本质是一个事件循环模型。

Rractor(或称 EventLoop)，它的核心逻辑是一个 loop 循环，使用伪代码描述如下：
```
void loop() {
  while(!stop) {
      foreach (task in tasks) {
        task();
      }

      // 1.取得下次定时任务的时间，与设定time_out去较大值，即若下次定时任务时间超过1s就取下次定时任务时间为超时时间，否则取1s
      int time_out = Max(1000, getNextTimerCallback());
      // 2.调用Epoll等待事件发生，超时时间为上述的time_out
      int rt = epoll_wait(epfd, fds, ...., time_out); 
      if(rt < 0) {
          // epoll调用失败。。
      } else {
          if (rt > 0 ) {
            foreach (fd in fds) {
              // 添加待执行任务到执行队列
              tasks.push(fd);
            }
          }
      } 
  }
}
```

在 rocket 里面，使用的是主从 Reactor 模型。
服务器有一个mainReactor和多个subReactor。

mainReactor由主线程运行，他作用如下：通过epoll监听listenfd的可读事件，当可读事件发生后，调用accept函数获取clientfd，然后随机取出一个subReactor，将cliednfd的读写事件注册到这个subReactor的epoll上即可。也就是说，mainReactor只负责建立连接事件，不进行业务处理，也不关心已连接套接字的IO事件。

subReactor通常有多个，每个subReactor由一个线程来运行。subReactor的epoll中注册了clientfd的读写事件，当发生IO事件后，需要进行业务处理。

### TimerEvent 定时任务
```
1. 指定时间点 arrive_time
2. interval ms
3. is_repeated
4. is_canceled
5. task

cancel()
cancelRepeated()
```

### Timer
定时器，是一个 TimerEvent 的集合
Timer 继承 FdEvent
```
addTimerEvent();
deleteTimerEvent();

onTimer();  // 发生了 IO 事件之后，需要执行的方法

resetArriveTime();

multimap 存储， TimerEvent <key(arrivetime), value(TimerEvent)>
```

### IO 线程
创建一个 IO 线程，它会帮我们执行：
* 创建一个新线程(pthread_create)
* 在新线程里面，创建一个 Eventloop，完成初始化
* 开启 loop
```
class {

  pthread_t m_thread;
  pid_t m_thread_id;
  EventLoop event_loop;
}
```
