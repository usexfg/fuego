// System/Timer.h
#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include <chrono>

namespace System {

class Dispatcher;

class Timer {
public:
    Timer();
    Timer(Dispatcher& dispatcher);
    Timer(Timer&& other);
    ~Timer();
    
    Timer& operator=(Timer&& other);
    
    void sleep(std::chrono::nanoseconds duration);

private:
    struct OperationContext {
        void* context;
        bool interrupted;
    };
    
    Dispatcher* dispatcher;
    void* context;
    int timer;
};

}

#endif // SYSTEM_TIMER_H
