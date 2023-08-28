#ifndef ROCKET_NET_CODER_ABSTRACT_PROTOCOL_H
#define ROCKET_NET_CODER_ABSTRACT_PROTOCOL_H

#include <memory>
#include <string>

namespace rocket {

struct AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol>{

public:
    using s_ptr = std::shared_ptr<AbstractProtocol>;

    virtual ~AbstractProtocol() {}

public:
    std::string m_req_id;   // 请求号，唯一标识一个请求或响应

};

};

#endif