/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#ifdef __APPLE__
#include <perspective/first.h>
#include <perspective/base.h>

namespace perspective
{

t_str
get_error_str()
{
    // handled by perror
    return t_str();
}

} // end namespace perspective
#endif
