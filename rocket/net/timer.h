#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include "rocket/net/fd_event.h"
#include "rocket/common/mutex.h"
#include "rocket/net/timer_event.h"

namespace rocket {

class Timer : public FdEvent {

public:
    Timer();

    ~Timer();

    void addTimerEvent(TimerEvent::s_ptr event);

    void deleteTimerEvent(TimerEvent::s_ptr evnet);

    void onTimer(); // 当发生了IO事件后， eventloop会执行这个函数

private:
    void resetArriveTime();

private:
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
    Mutex m_mutex;
};

};

#endif