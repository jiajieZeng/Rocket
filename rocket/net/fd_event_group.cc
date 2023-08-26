#include "rocket/net/fd_event_group.h"

namespace rocket {

static FdEventGroup* g_fd_event_group = NULL;

FdEventGroup::FdEventGroup(int size) : m_size(size) {
    for (int i = 0; i < m_size; ++i) {
        m_fd_event_group.push_back(new FdEvent(i));
    }
}

FdEventGroup::~FdEventGroup() {
    for (int i = 0; i < m_size; ++i) {
        if (m_fd_event_group[i] != NULL) {
            delete m_fd_event_group[i];
            m_fd_event_group[i] = NULL;
        }
    }
    m_size = 0;
}

FdEvent* FdEventGroup::getFdEvent(int fd) {
    ScopeMutex<Mutex> lock(m_mutex);
    if (fd < m_size) {
        lock.unlock();
        return m_fd_event_group[fd];
    }

    int new_size = int(fd * 1.5);
    for (int i = m_size; i < new_size; i++) {
        m_fd_event_group.push_back(new FdEvent(i));
    }
    lock.unlock();
    return m_fd_event_group[fd];
}


FdEventGroup* FdEventGroup::GetFdEventGroup() {
    if (g_fd_event_group != NULL) {
        return g_fd_event_group;
    }
    g_fd_event_group = new FdEventGroup(128);
    return g_fd_event_group;
}

};