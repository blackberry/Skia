/*
 * Copyright (C) 2014 BlackBerry Limited.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GentlTrace.h"

namespace Gentl {

Trace::Trace()
{
#if GENTL_TRACE
#endif
}

Trace::~Trace()
{
#if GENTL_TRACE
#endif
}

void Trace::setDirectTracePoint(void*)
{
#if GENTL_TRACE
#endif
}

void Trace::setCalculatedTracePoint(intptr_t*, intptr_t*)
{
#if GENTL_TRACE
#endif
}

}
