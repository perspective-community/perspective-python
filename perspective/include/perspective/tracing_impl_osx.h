/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#pragma once
#include <perspective/first.h>

#ifdef __APPLE__
extern "C" {
__attribute__((__constructor__)) void th_trace_init();
__attribute__((__destructor__)) void th_trace_fini();
}
#endif
