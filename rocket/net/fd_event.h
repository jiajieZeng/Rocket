#ifndef ROCKET_NET_FD_EVENT_H
#define ROCKET_NET_FD_EVENT_H

#include <functional>

#include <sys/epoll.h>
namespace rocket {

class FdEvent {

public:

    enum TriggerEvent {
        IN_EVENT = EPOLLIN,     //读
        OUT_EVENT = EPOLLOUT,   //写
    };

    FdEvent(int fd);
    
    ~FdEvent();

    std::function<void()> handler(TriggerEvent event_type);     // 根据event_type返回一个函数

    void listen(TriggerEvent event_type, std::function<void()> callback);   // 监听函数

    int getFd() const {
        return m_fd;
    }

    epoll_event getEpollEvent() {
        return m_listen_events;
    }

protected:
    int m_fd {-1};

    epoll_event m_listen_events;     //监听的事件

    std::function<void()> m_read_callback;
    std::function<void()> m_write_callback;
};

};

#endif