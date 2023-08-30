#include <sys/timerfd.h>
#include <string.h>
#include <vector>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"


namespace rocket {

Timer::Timer() : FdEvent() {

    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    DEBUGLOG("timer m_fd=%d", m_fd);

    // 把fd可读事件放到了eventloop上监听
    listen(FdEvent::IN_EVENT, std::bind(&Timer::onTimer, this));    // 监听可读事件，直接绑定到onTimer

}

Timer::~Timer() {

}

void Timer::onTimer() {
    // 处理缓冲区数据，防止下一次继续出发可读事件
    char buf[8];
    while (true) {
        if ((read(m_fd, buf, 8) == -1) && errno == EAGAIN) {
            break;
        }
    }

    // 执行定时任务
    int64_t now = getNowMs();
    std::vector<TimerEvent::s_ptr> tmps;
    std::vector<std::pair<int64_t, std::function<void()>>> tasks;

    ScopeMutex<Mutex> lock(m_mutex);
    auto it = m_pending_events.begin();

    for (it = m_pending_events.begin(); it != m_pending_events.end(); ++it) {
        // 需要执行
        if ((*it).first <= now) {
            if (!(*it).second->isCanceled()) {
                tmps.push_back((*it).second);
                tasks.push_back(std::make_pair((*it).second->getArriveTime(), (*it).second->getCallBack()));
            } else {
                break;
            }
        }
    }

    // 擦掉
    m_pending_events.erase(m_pending_events.begin(), it);
    lock.unlock();

    // 需要把重复的Event再次添加进去
    for (auto i = tmps.begin(); i != tmps.end(); ++i) {
        if((*i)->isRepeated()) {
            // 调整 arriveTime
            (*i)->resetArriveTime();
            addTimerEvent((*i));
        }
    }

    resetArriveTime();

    for (auto i: tasks) {
        if (i.second) {
            DEBUGLOG("onTimer run");
            i.second();
        }
    }
}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
    bool is_reset_timerfd = false;
    
    ScopeMutex<Mutex> lock(m_mutex);
    if (m_pending_events.empty()) {
        is_reset_timerfd = true;    // 设置事件，否则任务不会触发
    } else {
        auto it = m_pending_events.begin();
        // 需要插入的定时任务的指定时间比当前任务队列里面的定时任务执行时间都要早
        if ((*it).second->getArriveTime() > event->getArriveTime()) {
            is_reset_timerfd = true;
        }
    }
    m_pending_events.emplace(event->getArriveTime(), event);
    DEBUGLOG("add timer event in timer");
    lock.unlock();

    if (is_reset_timerfd) {
        resetArriveTime();
    }

}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event) {
    event->setCanceled(true);

    ScopeMutex<Mutex> lock(m_mutex);
    auto begin = m_pending_events.lower_bound(event->getArriveTime());
    auto end = m_pending_events.upper_bound(event->getArriveTime());
    auto it = begin;
    for (; it != end; it++) {
        if (it->second == event) {
            break;
        }
    }

    if (it != end) {
        m_pending_events.erase(it);
    }
    lock.unlock();
    DEBUGLOG("successfully delete TimerEvent at arrive time %lld", event->getArriveTime());

}

void Timer::resetArriveTime() {
    ScopeMutex<Mutex> lock(m_mutex);
    auto tmp = m_pending_events;
    lock.unlock();
    
    if (tmp.empty()) {
        return;
    }
    
    int64_t now = getNowMs();
    auto it = tmp.begin();
    int64_t interval = 0;
    if (it->second->getArriveTime() > now) {
        interval = it->second->getArriveTime() - now; 
    } else {
        interval = 100;     // 拯救过期任务
    }
    
    timespec ts;
    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;

    itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_value = ts;

    int rt = timerfd_settime(m_fd, 0, &value, NULL);
    if (rt != 0) {
        ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
    }
    DEBUGLOG("timer reset to %lld", now + interval);
}

};