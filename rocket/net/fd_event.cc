#include <string.h>
#include <fcntl.h>
#include "rocket/net/fd_event.h"
#include "rocket/common/log.h"

namespace rocket {

FdEvent::FdEvent(int fd) : m_fd(fd) {
    memset(&m_listen_events, 0, sizeof(m_listen_events));
}

FdEvent::FdEvent() {
    memset(&m_listen_events, 0, sizeof(m_listen_events));
}

FdEvent::~FdEvent() {

}

std::function<void()> FdEvent::handler(TriggerEvent event_type) {
    if (event_type == TriggerEvent::IN_EVENT) {
        return m_read_callback;  //执行回调函数
    } else if (event_type == TriggerEvent::OUT_EVENT) {
        return m_write_callback;
    } 
    return nullptr;
}

void FdEvent::listen(TriggerEvent event_type, std::function<void()> callback) {
    if (event_type == TriggerEvent::IN_EVENT) {
        m_listen_events.events |= EPOLLIN;
        m_read_callback = callback;
    } else {
        m_listen_events.events |= EPOLLOUT;
        m_write_callback = callback;
    }
    m_listen_events.data.ptr = this;
}

void FdEvent::setNonBlock() {
    int flag = fcntl(m_fd, F_GETFL, 0);
    if (flag & O_NONBLOCK) {
        return;
    }

    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);

}

void FdEvent::cancle(TriggerEvent event_type) {
    if (event_type == TriggerEvent::IN_EVENT) {
        DEBUGLOG("BEFORE m_listen_events.events=%d", (int)m_listen_events.events);
        m_listen_events.events &= (~EPOLLIN);
        DEBUGLOG("LAST m_listen_events.events=%d", (int)m_listen_events.events);
    } else {
        DEBUGLOG("BEFORE m_listen_events.events=%d", (int)m_listen_events.events);
        m_listen_events.events &= (~EPOLLOUT);
        DEBUGLOG("LAST m_listen_events.events=%d", (int)m_listen_events.events);
    }
}

};