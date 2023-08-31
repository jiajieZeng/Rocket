#include "rocket/common/runtime.h"

namespace rocket {

thread_local RunTime* t_run_time = NULL;

RunTime* RunTime::GetRunTime() {
    if (t_run_time == NULL) {
        t_run_time = new RunTime();
    }
    return t_run_time;
}

}