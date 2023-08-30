#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/common/error_code.h"

namespace rocket {


RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    m_client = std::make_shared<TcpClient>(m_peer_addr);
}

RpcChannel::~RpcChannel() {
    DEBUGLOG("~RpcChannel");
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, 
                        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                        google::protobuf::Message* response, google::protobuf::Closure* done) {
    
    std::shared_ptr<rocket::TinyPBProtocol> req_protocol = std::make_shared<rocket::TinyPBProtocol>();
    RpcController* my_controller = dynamic_cast<RpcController*>(controller);
    if (my_controller == NULL) {
        ERRORLOG("failed Callmethod, RpcController convert error");
        return;
    }
 

    if (my_controller->GetMsgId().empty()) {
        req_protocol->m_msg_id = MsgIDUtil::GenMsgID();
        my_controller->SetMsgId(req_protocol->m_msg_id);
    } else {
        req_protocol->m_msg_id = my_controller->GetMsgId();
    }

    req_protocol->m_method_name = method->full_name();
    INFOLOG("%s | call method name [%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str());

    if (!m_is_init) {
        std::string err_info = "RpcChannel not init";
        my_controller->SetError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("%s | %s, RpcChannel not init [%s]", req_protocol->m_msg_id.c_str(), err_info.c_str(), 
                request->ShortDebugString().c_str());
        return;
    }

    if (!request->SerializeToString(&(req_protocol->m_pb_data))) {
        std::string err_info = "failed to serialize";
        my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
        ERRORLOG("%s | %s, origin request [%s]", req_protocol->m_msg_id.c_str(), err_info.c_str(), 
                request->ShortDebugString().c_str());
        return;
    }

    s_ptr channel = shared_from_this();

    m_timer_event = std::make_shared<TimerEvent>(my_controller->GetTimeout(), false, [my_controller, channel]() mutable {
        my_controller->StartCancel();
        my_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(my_controller->GetTimeout()));
        if (channel->GetClosure()) {
            channel->GetClosure()->Run();
        }
        channel.reset();
    }); 

    m_client->addTimerEvent(m_timer_event);

    m_client->connect([req_protocol, channel]() mutable {

        RpcController* my_controller = dynamic_cast<RpcController*>(channel->GetController());
        if (channel->GetTcpClient()->getConnectErrorCode() != 0) {
            my_controller->SetError(channel->GetTcpClient()->getConnectErrorCode(), 
                channel->GetTcpClient()->getConnectErrorInfo());

            ERRORLOG("%s | connect error, error code[%d], error info[%s], peer addr[%s]", 
                req_protocol->m_msg_id.c_str(), my_controller->GetErrorCode(), my_controller->GetErrorInfo().c_str(), 
                channel->GetTcpClient()->getPeerAddr()->toString().c_str());
            return;
        }

        channel->GetTcpClient()->writeMessage(req_protocol, [req_protocol, channel, my_controller](AbstractProtocol::s_ptr msg_ptr) mutable {
            INFOLOG("%s | send request success. call method name[%s], peer addr[%s], local addr[%s]",
                req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str(), 
                channel->GetTcpClient()->getPeerAddr()->toString().c_str(), channel->GetTcpClient()->getLocalAddr()->toString().c_str());
            
            channel->GetTcpClient()->readMessage(req_protocol->m_msg_id, [channel, my_controller](AbstractProtocol::s_ptr msg) mutable {
                std::shared_ptr<TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol> (msg);
                INFOLOG("%s | success get rpc response, call method name[%s], peer addr[%s], local addr[%s]", 
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->GetTcpClient()->getPeerAddr()->toString().c_str(), channel->GetTcpClient()->getPeerAddr()->toString().c_str());

                // 当成功读取回包，取消定时任务
                channel->GetTimerEvnet()->setCanceled(true);

                // RpcController* my_controller = dynamic_cast<RpcController*>(channel->GetController());
                if (!(channel->GetResponse()->ParseFromString(rsp_protocol->m_pb_data))) {
                    ERRORLOG("%s | deserialize error", rsp_protocol->m_msg_id.c_str());
                    my_controller->SetError(ERROR_FAILED_SERIALIZE, "deserialize error");
                    return;
                }

                if (rsp_protocol->m_err_code != 0) {
                    ERRORLOG("%s | call rpc method[%s] failed, error code[%d], error info[%s]", 
                        rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(), 
                        rsp_protocol->m_err_code, rsp_protocol->m_err_info.c_str());
                   
                    my_controller->SetError(rsp_protocol->m_err_code, rsp_protocol->m_err_info);
                    return;
                }

                INFOLOG("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]",
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->GetTcpClient()->getPeerAddr()->toString().c_str(), channel->GetTcpClient()->getPeerAddr()->toString().c_str());

                if ((!my_controller->IsCanceled()) && channel->GetClosure()) {
                    channel->GetClosure()->Run();
                }

                channel.reset();    // shared_ptr ref_count -1 
            });

        });
    });

}

void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr rsp, closure_s_ptr done) {
    if (m_is_init) {
        return;
    }
    m_controller = controller;
    m_request = req;
    m_response = rsp;
    m_closure = done;
    m_is_init = true;
}

google::protobuf::RpcController* RpcChannel::GetController() {
    return m_controller.get(); 
}

google::protobuf::Message* RpcChannel::GetRequest() {
    return m_request.get();
}

google::protobuf::Message* RpcChannel::GetResponse() {
    return m_response.get(); 
}

google::protobuf::Closure* RpcChannel::GetClosure() {
    return m_closure.get(); 
}

TcpClient* RpcChannel::GetTcpClient() {
    return m_client.get();
}

TimerEvent::s_ptr RpcChannel::GetTimerEvnet() {
    return m_timer_event;
}

}