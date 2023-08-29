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
#include "order.pb.h"

void test_tcp_client() {
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr> ("127.0.0.1", 12345);
    rocket::TcpClient client(addr);
    client.connect([addr, &client](){
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        // message->info = "hello rocket!";
        message->m_req_id = "99998888";
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
            DEBUGLOG("req_id [%s], get response %s", message->m_req_id.c_str(), message->m_pb_data.c_str());

            makeOrderResponse response;
            if (!response.ParseFromString(message->m_pb_data)) {
                ERRORLOG("deserialize error");
                return;
            }

            DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());

        });

    });
}

int main() {
    rocket::Config::SetGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();

    test_tcp_client();
    
    return 0;
}