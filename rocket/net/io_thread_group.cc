#include "rocket/net/io_thread_group.h"
#include "rocket/common/log.h"

namespace rocket {

IOThreadGroup::IOThreadGroup(int size) : m_size(size) {
    m_io_thread_group.resize(size);
    for (int i = 0; i < size; ++i) {
        m_io_thread_group[i] = new IOThread();
    }
}

IOThreadGroup::~IOThreadGroup() {

}

void IOThreadGroup::start() {
    // for (int i = 0; i < m_size; i++) {
    //     m_io_thread_group[i]->start();
    // }
    for (auto it: m_io_thread_group) {
        it->start();
    }
}

void IOThreadGroup::join() {
    for (auto it: m_io_thread_group) {
        it->join();
    }
}

IOThread* IOThreadGroup::getIOThread() {
    if (m_index == m_size || m_index == -1) {
        m_index = 0;
    }
    return m_io_thread_group[m_index++];
}


};
