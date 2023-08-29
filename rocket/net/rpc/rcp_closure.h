#ifndef ROCKET_NET_RPC_RPC_CLOSURE_H
#define ROCKET_NET_RPC_RPC_CLOSURE_H

#include <functional>
#include <google/protobuf/stubs/callback.h>

namespace rocket {

class RpcClosure : public google::protbobuf::Closure {

public:
    void Run() override {
        if (m_cb != nullptr) {
            m_cb();
        }
    }

private:
    std::function<void()> m_cb {nullptr};

};

}

#endif