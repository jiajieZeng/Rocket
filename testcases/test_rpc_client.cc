#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <memory>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_closure.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "order.pb.h"

void test_tcp_client() {
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr> ("127.0.0.1", 12345);
    rocket::TcpClient client(addr);
    client.connect([addr, &client](){
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        // message->info = "hello rocket!";
        message->m_msg_id = "99998888";
        message->m_pb_data = "test pb data";
        makeOrderRequest request;
        request.set_price(100);
        request.set_goods("apple");
        if (!request.SerializeToString(&(message->m_pb_data))) {
            ERRORLOG("serialize error");
            return;
        }

        message->m_method_name = "Order.makeOrder";

        client.writeMessage(message, [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
            DEBUGLOG("send message success request[%s]", request.ShortDebugString().c_str());
        });

        std::string msg = "99998888";
        client.readMessage(msg, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
            // DEBUGLOG("send message success");
            std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id [%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());

            makeOrderResponse response;
            if (!response.ParseFromString(message->m_pb_data)) {
                ERRORLOG("deserialize error");
                return;
            }

            DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());

        });

    });
}

void test_rpc_channel() {
    NEWRPCCHANNEL("127.0.0.1:12345", channel);
    NEWMESSAGE(makeOrderRequest, request);
    NEWMESSAGE(makeOrderResponse, response);
    request->set_price(100);
    request->set_goods("apple");

    NEWRPCCONTROLLER(controller);
    controller->SetMsgId("99998888");

    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>(nullptr, [request, response, channel, controller] () mutable {
        if (controller->GetErrorCode() == 0) {
            INFOLOG("TEST RPC CHANNEL call rpc success, request[%s], response[%s]", 
                request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
            // 执行业务逻辑
        } else {
            ERRORLOG("TEST RPC CHANNEL call rpc failed, request[%s], response[%d], error info[%s]",
                request->ShortDebugString().c_str(),
                controller->GetErrorCode(), controller->GetErrorInfo().c_str());
        }
        INFOLOG("now exit eventloop");
        // channel->GetTcpClient()->stop();
        channel.reset();
    });


    controller->SetTimeout(10000);
    CALLRPC("127.0.0.1:12345", Order_Stub, makeOrder, controller, request, response, closure);
}

int main() {
    rocket::Config::SetGlobalConfig(NULL);
    rocket::Logger::InitGlobalLogger(0);

    test_rpc_channel();

    DEBUGLOG("exit test rpc client");

    return 0;
}
