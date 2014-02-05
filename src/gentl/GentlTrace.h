/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GentlTrace_h
#define GentlTrace_h

#include <stdint.h>

namespace Gentl {

class Trace {
public:
    Trace();
    ~Trace();

    template<typename FunctionType>
    Trace(FunctionType function)
    {
        setCalculatedTracePoint((intptr_t*) &function);
    }

    template<typename ObjectType, typename MethodType>
    Trace(ObjectType object, MethodType method)
    {
        setCalculatedTracePoint((intptr_t*) &method, (intptr_t*) &object);
    }

private:
    void setDirectTracePoint(void* address);
    void setCalculatedTracePoint(intptr_t* function, intptr_t* object = 0);

    void* fAddress;
};

}

#endif // GentlTrace_h
