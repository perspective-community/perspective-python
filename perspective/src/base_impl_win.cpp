/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#ifdef WIN32
#include <perspective/first.h>
#include <perspective/base.h>

namespace perspective
{

t_str
get_error_str()
{
    DWORD errid = ::GetLastError();

    if (errid == 0)
        return t_str();

    LPSTR buf = 0;

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errid,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf,
        0,
        NULL);

    t_str message(buf, size);
    LocalFree(buf);

    return message;
}

} // end namespace perspective
#endif