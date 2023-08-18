#ifndef ROCKET_COMMON_MUTEXT_H
#define ROCKET_COMMON_MUTEXT_H

#include <pthread.h>

namespace rocket {

template <class T>
class ScopeMutex {

public:
    ScopeMutex(T& mutex) : m_mutex(mutex) {
        m_mutex.lock();
        m_is_lock = true;
    }

    ~ScopeMutex() {
        m_is_lock = false;
        m_mutex.unlock();
    }

    void lock() {
        if (!m_is_lock) {
            m_mutex.lock();
            m_is_lock = true;
        }
    }

    void unlock() {
        if (m_is_lock) {
            m_is_lock = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    
    bool m_is_lock {false};

};

class Mutex {

public:
    Mutex() {   
        pthread_mutex_init(&m_mutex, NULL);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t* getMutex() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

};

#endif