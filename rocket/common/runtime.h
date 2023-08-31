#ifndef ROCKET_COMMON_RUNTIME_H
#define ROCKET_COMMON_RUNTIME_H

#include <string>

namespace rocket {

class RunTime {

public:
    static RunTime* GetRunTime();

public:
    std::string m_msg_id;
    std::string m_method_name;

};


}

#endif