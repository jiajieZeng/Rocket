#include <string.h>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/common/log.h"

namespace rocket {

TcpBuffer::TcpBuffer(int size) : m_size(size) {
    m_buffer.resize(size);
}

TcpBuffer::~TcpBuffer() {

}

int TcpBuffer::readAble() {
    return m_write_index - m_read_index;
}

int TcpBuffer::writeAble() {
    return  m_size - m_write_index;
}

int TcpBuffer::readIndex() {
    return m_read_index;
}

int TcpBuffer::writeIndex() {
    return m_write_index;
}

void TcpBuffer::writeToBuffer(char const* buf, int size) {
    if (size > writeAble()) {
        // 不够空间大小，调整buffer
        int new_size = (int)(1.5 * (m_write_index + size));
        resizeBuffer(new_size);
    }

    memcpy(&m_buffer[m_write_index], buf, size);
}

void resizeBuffer(int new_size) {
    std::vector<char> tmp(new_size);

    int count = std::min(new_size, readAble());

    memcpy(&tmp[0], &m_buffer[m_read_index], count);
    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = m_read_index + count;
    m_size = new_size;
}

void TcpBuffer::readFromBuffer(std::vector<char>& re) {
    if (readAble() == 0) {
        return;
    }
    
    int read_size = readAble() > size ? size : readAble();
    std::vector<char> tmp(read_size);
    
    memcpy(&tmp[0], &m_buffer[m_read_index], read_size);
    re.swap(tmp);
    m_read_index += read_size;

    adjustBuffer();
}

void TcpBuffer::adjustBuffer() {
    // 如果 m_read_size 超过三分之一，调整
    if (m_read_index < (int)(m_size / 3)) {
        return;
    }

    std::vector<char> buffer(m_size);
    int count = readAble();

    memcpy(&buffer[0], &m_buffer[m_read_index], count);
    m_buffer.swap(buffer);
    m_read_index = 0;
    m_write_index = m_read_index + count;
    
    buffer.clear();
}   

void TcpBuffer::moveReadIndex(int size) {
    int j = m_read_index + size;
    if (j >= m_size) {
        ERRORLOG("moveReadIndex error, invalid size %d, old_read_size %d, buffer size %d", size, m_read_index, m_size);
    }

    m_read_index = j;
    adjustBuffer();
}

void TcpBuffer::moveWriteIndex(int size) {
    int j = m_write_index + size;
    if (j >= m_size) {
        ERRORLOG("moveWriteIndex error, invalid size %d, old_write_size %d, buffer size %d", size, m_write_index, m_size);
    }

    m_write_index = j;
    adjustBuffer();     //  似乎不需要调整
}

};