# Rocket

rocket 是基于 C++11 开发的一款多线程的异步 RPC 框架，它旨在高效、简洁的同时，又保持至极高的性能。

rocket 同样是基于主从 Reactor 架构，底层采用 epoll 实现 IO 多路复用。应用层则基于 protobuf 自定义 rpc 通信协议，同时也将支持简单的 HTTP 协议。

这个项目会以龟速进行开发，可能很久才commit一次。

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

Logger 日志器 1.提供打印日志的方法 2.设置日志输出的路径