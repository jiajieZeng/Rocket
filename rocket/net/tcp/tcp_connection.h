#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include <vector>
#include <map>  
#include <functional>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/rpc/rpc_dispatcher.h"

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

    TcpConnection(EventLoop* eventloop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, 
        NetAddr::s_ptr local_addr, TcpConnetionType type = TcpConnectionByServer);

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

    // 启动监听可写事件
    void listenWrite();

    // 启动监听可读事件
    void listenRead();

    void pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    void pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);    

    NetAddr::s_ptr getLocalAddr();
    NetAddr::s_ptr getPeerAddr();

private:
    EventLoop* m_event_loop {NULL};   // 当前持有该链接的 eventloop 
    int m_fd {0};
    NetAddr::s_ptr m_peer_addr;
    NetAddr::s_ptr m_local_addr;

    TcpBuffer::s_ptr m_in_buffer;   // 接收缓冲区
    TcpBuffer::s_ptr m_out_buffer;  // 发送缓冲区

    FdEvent* m_fd_event {NULL};     

    TcpState m_state;
    
    TcpConnetionType m_connection_type {TcpConnectionByServer};

    AbstractCoder* m_coder {NULL};

    // std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>
    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones;
    
    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)> > m_read_dones;

};

};

#endif