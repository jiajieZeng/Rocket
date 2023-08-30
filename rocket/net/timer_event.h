#ifndef ROCKET_NET_TIMER_EVENT_H
#define ROCKET_NET_TIMER_EVENT_H

#include <functional>
#include <memory>

namespace rocket {

class TimerEvent {

public:
    using s_ptr = std::shared_ptr<TimerEvent>;

    TimerEvent(int64_t interval, bool is_repeated, std::function<void()> cb);

    int64_t getArriveTime() const {
        return m_arrive_time;
    }

    void setCanceled(bool value) {
        m_is_canceled = value;
    }

    bool isCanceled() {
        return m_is_canceled;
    }

    bool isRepeated() {
        return m_is_repeated;
    }

    std::function<void()> getCallBack() {
        return m_task;
    }

    void resetArriveTime();

private:
    int64_t m_arrive_time;  // ms
    int64_t m_interval;     // ms
    bool m_is_repeated {false};
    bool m_is_canceled {false};
    
    std::function<void()> m_task;

    
};

};

#endif