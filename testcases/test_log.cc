#include <pthread.h>
#include <iostream>
#include <unistd.h>

#include "rocket/common/log.h"
#include "rocket/common/config.h"

void *fun(void* arg) {
    int i = 20;
    while (i--) {
        DEBUGLOG("debug this is thread %s", "fun");
        INFOLOG("info this is thread %s", "fun");
    }
    return NULL;
}

int main() {
    rocket::Config::SetGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();
    pthread_t thread;
    pthread_create(&thread, NULL, fun, NULL);
    int i = 20;
    while (i--) {
        DEBUGLOG("test debug log %s", "11");
        INFOLOG("test info log %s", "11");
    }
    pthread_join(thread, NULL);
    return 0;
}