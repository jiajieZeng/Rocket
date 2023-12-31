#include <unistd.h>
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinypb_coder.h"

namespace rocket {

TcpConnection::TcpConnection(EventLoop* eventloop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, 
        NetAddr::s_ptr local_addr, TcpConnetionType type /*= TcpConnectionByServer*/)
    : m_event_loop(eventloop), m_fd(fd), m_peer_addr(peer_addr), m_local_addr(local_addr),
        m_state(NotConnected) , m_connection_type(type){
    m_in_buffer = std::make_shared<TcpBuffer> (buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer> (buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlock();
    m_coder = new TinyPBCoder();

    if (m_connection_type == TcpConnectionByServer) {
        listenRead();
    }

}

TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection ");
    if (m_coder) {
        delete m_coder;
        m_coder = NULL;
    }
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
    if (m_connection_type == TcpConnectionByServer) {
        // 将 RPC 请求执行业务逻辑， 获取 RPC 逻辑， 再把 RPC 响应发送回去

        std::vector<AbstractProtocol::s_ptr> result;
        std::vector<AbstractProtocol::s_ptr> reply_messages;
        m_coder->decode(result, m_in_buffer);
        for (size_t i = 0; i < result.size(); i++) {
            // 针对每一个请求，调用 Rpc 方法，获取响应 message
            // 将响应 message 放入到发送缓冲区，监听可写事件回包
            INFOLOG("successfully get request[%s] from client [%s]", result[i]->m_msg_id.c_str(), m_peer_addr->toString().c_str());
            std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();
            // message->m_pb_data = "hello. this is rocket rpc test data";
            // message->m_msg_id = result[i]->m_msg_id;
            RpcDispatcher::GetRpcDispatcher()->dispatch(result[i], message, this); 
            reply_messages.push_back(message);
        }
        
        m_coder->encode(reply_messages, m_out_buffer);

        // 监听写回调函数
        listenWrite();
    } else {
        std::vector<AbstractProtocol::s_ptr> result;
        m_coder->decode(result, m_in_buffer);

        for (size_t i = 0; i < result.size(); i++) {
            std::string msg_id = result[i]->m_msg_id;
            auto it = m_read_dones.find(msg_id);
            if (it != m_read_dones.end()) {
                it->second(result[i]);
            }
        }

    }

}

void TcpConnection::onWrite() {
    // 将当前 out_buffer 里面的数据全部发送给 peer
    if (m_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr [%s], clientfd [%d]", 
            m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    if (m_connection_type == TcpConnectionByClient) {
        // 将 message encode 得到字节流
        // 将字节流写入 buffer 然后全部发送
        std::vector<AbstractProtocol::s_ptr> messages;
        for (size_t i = 0; i < m_write_dones.size(); i++) {
            messages.push_back(m_write_dones[i].first);
        }
        m_coder->encode(messages, m_out_buffer);
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
        m_fd_event->cancel(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

    if (m_connection_type == TcpConnectionByClient) {
        for (size_t i = 0; i < m_write_dones.size(); i++) {
            m_write_dones[i].second(m_write_dones[i].first);
        }
        m_write_dones.clear();
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

    m_fd_event->cancel(FdEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::OUT_EVENT);
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

void TcpConnection::listenWrite() {
    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::listenRead() {
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
    m_event_loop->addEpollEvent(m_fd_event);

}


void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_write_dones.push_back(std::make_pair(message, done));
}


void TcpConnection::pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    // m_read_dones.insert(std::make_pair(msg_id, done));
    m_read_dones[msg_id] = done;
}

NetAddr::s_ptr TcpConnection::getLocalAddr() {
    return m_local_addr;
}

NetAddr::s_ptr TcpConnection::getPeerAddr() {
    return m_peer_addr;
}

};