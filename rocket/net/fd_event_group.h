#ifndef ROCKET_NET_FD_EVENT_GROUP_H
#define ROCKET_NET_FD_EVENT_GROUP_H

#include <vector>
#include "rocket/net/fd_event.h"
#include "rocket/common/mutex.h"

namespace rocket {

class FdEventGroup {

public:
    FdEventGroup(int size);
    
    ~FdEventGroup();

    FdEvent* getFdEvent(int fd);

    static FdEventGroup* GetFdEventGroup();

private:
    int m_size {0};

    std::vector<FdEvent*> m_fd_event_group;

    Mutex m_mutex;
};

};

#endif