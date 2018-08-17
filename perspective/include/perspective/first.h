/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#ifndef PSP_ENABLE_WASM
#define PSP_PARALLEL_FOR
#endif

#ifdef PSP_ENABLE_PYTHON
#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numpy.hpp>
#include <numpy/arrayobject.h>
#endif

#pragma once
#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // ifndex nominmax
#endif // win32

#ifdef PSP_VERIFY
#define PSP_STORAGE_VERIFY
#define PSP_COLUMN_VERIFY
#define PSP_TABLE_VERIFY
#define PSP_GNODE_VERIFY
#define PSP_TABLE_TRACE
//#define PSP_DEBUG_HELPER
//#define PSP_MPROTECT
//#define PSP_DBG_MALLOC
#endif

// Remove once we are c++11 everywhere
#define TBB_DEPRECATED_MUTEX_COPYING 1
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define _GLIBCXX_USE_NANOSLEEP 1
#include <string>
#include <iostream>

#ifndef PSP_DEBUG_HELPER
#define PSP_MALLOC malloc
#define PSP_FREE free
#endif

#ifdef PSP_DBG_MALLOC
#define PSP_MALLOC psp_dbg_malloc
#define PSP_FREE psp_dbg_free
#endif

#ifdef PSP_MPROTECT
#define PSP_MALLOC psp_page_aligned_malloc
#define PSP_FREE psp_page_aligned_free
#endif
