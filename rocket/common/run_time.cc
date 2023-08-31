#include "rocket/common/run_time.h"

namespace rocket {

thread_local RunTime* t_run_time = NULL;

RunTime* RunTime::GetRunTime() {
    if (t_run_time == NULL) {
        t_run_time = new RunTime();
    }
    return t_run_time;
}

RpcInterface* RunTime::getRpcInterface() {
  return m_rpc_interface;
}

}
