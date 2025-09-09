// System/Dispatcher.h
#ifndef SYSTEM_DISPATCHER_H
#define SYSTEM_DISPATCHER_H

#include "NativeContext.h"
#include "NativeContextGroup.h"

namespace System {

    class Dispatcher {
    public:
        Dispatcher() = default;
        virtual ~Dispatcher() = default;
        NativeContext& getReusableContext() { static NativeContext ctx; return ctx; }
        void pushContext(NativeContext* context) {}
        void pushReusableContext(NativeContext& context) {}
        void interrupt() {}
        void interrupt(NativeContext* context) {}
        void clear() {}
        void dispatch() {}
    };
}

#endif // SYSTEM_DISPATCHER_H
