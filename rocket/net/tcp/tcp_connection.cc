#include <unistd.h>
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"

namespace rocket {

TcpConnection::TcpConnection(EventLoop* eventloop, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
    : m_event_loop(eventloop), m_fd(fd), m_peer_addr(peer_addr), m_state(NotConnected) {
    m_in_buffer = std::make_shared<TcpBuffer> (buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer> (buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlock();
    // 发生可读事件，调用读回调函数
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));  

    m_event_loop->addEpollEvent(m_fd_event); 
}

TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection ");
}


void TcpConnection::onRead() {
    // 从 socket 缓冲区，调用系统的 read 函数读取字节到 in_buffer 里面
    if (m_state != Connected) {
        ERRORLOG("onRead error, client has already disconnected, addr [%s], clientfd [%d]", 
            m_peer_addr->toString().c_str(), 
            m_fd);
        return;
    }

    bool is_read_all = false;
    bool is_closed = false;
    while (!is_read_all) {

        if (m_in_buffer->writeAble() == 0) {
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
        }

        int read_count = m_in_buffer->writeAble();
        int write_index = m_in_buffer->writeIndex();

        int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
        DEBUGLOG("successfully read %d bytes from addr [%s], client fd [%d]", rt, m_peer_addr->toString().c_str(), m_fd);

        if (rt > 0) {
            m_in_buffer->moveWriteIndex(rt);
            if (rt == read_count) {
                continue;
            } else if (rt < read_count) {
                is_read_all = true;
                break;
            }
        } else if (rt == 0) {
            // 对等端关闭连接
            is_closed = true;
            break;
        } else if (rt == -1 && errno == EAGAIN) {
            is_read_all = true;
            break;
        }

    }

    if (is_closed) {
        // TODO: 处理关闭连接
        DEBUGLOG("peer closed, peer addr [%s], clientfd [%d]", m_peer_addr->toString().c_str(), m_fd);
        clear();
        return;     // 找了四个小时，终于找到忘记return
    }

    if (!is_read_all) {
        ERRORLOG("not read all data");
    }

    // TODO: 简单的echo, 后面补充 RPC 协议的解析
    execute();

}

void TcpConnection::execute() {
    // 将 RPC 请求执行业务逻辑， 获取 RPC 逻辑， 再把 RPC 响应发送回去
    std::vector<char> tmp;
    int size = m_in_buffer->readAble();
    tmp.resize(size);
    m_in_buffer->readFromBuffer(tmp, size);

    std::string msg = "";
    for (auto it: tmp) {
        msg += it;
    }

    INFOLOG("successfully get request[%s] from client [%s]", msg.c_str(), m_peer_addr->toString().c_str());

    m_out_buffer->writeToBuffer(msg.c_str(), msg.length());
    // 监听写回调函数
    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));

    m_event_loop->addEpollEvent(m_fd_event);

}

void TcpConnection::onWrite() {
    // 将当前 out_buffer 里面的数据全部发送给 peer
    if (m_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr [%s], clientfd [%d]", 
            m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    bool is_write_all = false; 
    while (true) {
        if (m_out_buffer->readAble() == 0) {
            DEBUGLOG("no date need to send to client [%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }

        int write_size = m_out_buffer->readAble();
        int read_index = m_out_buffer->readIndex();

        int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

        if (rt >= write_size) {
            DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }
        if (rt == -1 && errno == EAGAIN) {
            // 发送缓冲区已满，不能再发送了
            // 等下次 fd 科协的时候再次发送数据
            ERRORLOG("write data error, errno == EAGAIN and rt == -1");
            break;
        }

    }

    if (is_write_all) {
        m_fd_event->cancle(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

}

void TcpConnection::setState(const TcpState state) {
    m_state = state;
}

TcpState TcpConnection::getState() {
    return m_state;
}

void TcpConnection::clear() {
    // 处理一些关闭连接后的清理动作
    if (m_state == Closed) {
        return;
    }

    m_fd_event->cancle(FdEvent::IN_EVENT);
    m_fd_event->cancle(FdEvent::OUT_EVENT);
    m_event_loop->deleteEpollEvent(m_fd_event);
    m_state = Closed;
}

void TcpConnection::shutdown() {
    if (m_state == Closed || m_state == NotConnected) {
        return;
    }

    // 设置为半关闭状态
    m_state = TcpState::HalfClosing;

    // 关闭读写，服务器不会再对这个 fd 进行读写操作
    // 发送 FIN 报文，触发四次挥手的第一个阶段
    // 当 fd 发生可读事件，但是可读数据为0时，即对端发送了 FIN 报文
    ::shutdown(m_fd, SHUT_RDWR);

}

void TcpConnection::setConnectionType(TcpConnetionType type) {
    m_connection_type = type;
}

};