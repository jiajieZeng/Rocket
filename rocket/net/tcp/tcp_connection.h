#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/io_thread.h"

namespace rocket {

enum TcpState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4,
};

enum TcpConnetionType {
    TcpConnectionByServer = 1,  // 作为服务端使用，代表跟对端客户端的连接
    TcpConnectionByClient = 2,  // 作为客户端使用，代表跟对端服务端的连接
};

class TcpConnection {

public:

    using s_ptr = std::shared_ptr<TcpConnection>; 

    TcpConnection(EventLoop* eventloop, int fd, int buffer_size, NetAddr::s_ptr peer_addr);

    ~TcpConnection();

    void onRead();

    void execute();

    void onWrite();

    void setState(const TcpState state);

    TcpState getState();

    void clear();

    // 服务器主动关闭连接
    void shutdown();

    void setConnectionType(TcpConnetionType type);

private:
    EventLoop* m_event_loop {NULL};   // 当前持有该链接的 eventloop 
    int m_fd {0};
    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    TcpBuffer::s_ptr m_in_buffer;   // 接收缓冲区
    TcpBuffer::s_ptr m_out_buffer;  // 发送缓冲区

    FdEvent* m_fd_event {NULL};     

    TcpState m_state;
    
    TcpConnetionType m_connection_type {TcpConnectionByServer};
};

};

#endif