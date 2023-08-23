#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include <vector>

/*
应用层的buffer
TCP是传输层的协议，传输的只是字节流，没有包的概念，所以其实没有粘包的概念
那么粘包概念应该是在应用层，按照HTTP的请求体其实就是一个请求包，按照HTTP协议的标准格式去将字节流进行拆解，
获取一个完整的HTTP请求包，叫请求包的组装。
在实际网络通讯过程中，有可能TCP接收到的数据不是一个完整的应用层的包，如果只接收到半个HTTP请求的包数据，
这时候半个请求包的字节流在buffer里面，读还是不读？读的话要怎样
所以引用应用层的buffer，先将数据暂存在buffer里面，等到数据完整，再去获取应用层的包

*/

namespace rocket {

class TcpBuffer {

public:

    using s_ptr = std::shared_ptr<TcpBuffer>;

    TcpBuffer(int size);

    ~TcpBuffer();

    int readAble();     // 返回可读字节数

    int writeAble();    // 返回可写字节数

    int readIndex();

    int writeIndex();

    void writeToBuffer(char const* buf, int size);

    void readFromBuffer(std::vector<char>& re, int size);

    void resizeBuffer(int new_size);

    void adjustBuffer();

    void moveReadIndex(int size);

    void moveWriteIndex(int size);

private:
    int m_read_index {0};
    int m_write_index {0};
    int m_size {0};

    std::vector<char> m_buffer;

};

};

#endif