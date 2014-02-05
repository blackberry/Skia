/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GentlTrace.h"

#include <sys/trace.h>

namespace Gentl {

// This method is enabled even when GENTL_TRACE is turned off so that it can be called from the unit tests.
void* addressOfFunctionOrMethod(intptr_t* function, intptr_t* object)
{
    intptr_t address = *function;

    // If this is a method pointer (ie. there is an object), the word after the
    // pointer tells whether it is a virtual function or not.
    intptr_t adjustment = object ? *(function + 1) : 0;

    // If the least-significant-bit of adjustment is set, this is a member
    // function pointer to a virtual function and we need to look at the
    // vtable. If it isn't set, it's pointer to a regular function or
    // non-virtual member function, so it's just an address.
    if (adjustment & 1) {
        // TODO: adjust the object pointer by adjustment, to account for
        // vtables appended due to multiple inheritance. Right now this breaks
        // if your class multiple inherits from two base classes that both
        // contain virtual functions. PR #140894
        char* vtable = *((char**) (*object));
        return *((void**) (address + vtable));
    }

    return (void*) address;
}

Trace::Trace()
{
#if GENTL_TRACE
    setDirectTracePoint(__builtin_return_address(0));
#endif
}

Trace::~Trace()
{
#if GENTL_TRACE
    trace_func_exit(fAddress, 0);
#endif
}

void Trace::setDirectTracePoint(void* address)
{
#if GENTL_TRACE
    fAddress = address;
    trace_func_enter(fAddress, 0);
#endif
}

void Trace::setCalculatedTracePoint(intptr_t* function, intptr_t* object)
{
#if GENTL_TRACE
    setDirectTracePoint(addressOfFunctionOrMethod(function, object));
#endif
}

}
