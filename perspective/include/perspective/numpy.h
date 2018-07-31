/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#if defined(PSP_ENABLE_PYTHON_JPM) || defined(PSP_ENABLE_PYTHON)

#pragma once
#include <perspective/first.h>
#include <perspective/raw_types.h>
#include <perspective/base.h>
#include <perspective/exports.h>
#define NO_IMPORT_ARRAY
#define PY_ARRAY_UNIQUE_SYMBOL _perspectiveNumpy
#include <numpy/arrayobject.h>

#ifdef PSP_ENABLE_PYTHON
namespace py = boost::python;
namespace np = boost::python::numpy;
#endif

namespace perspective
{

#ifdef PSP_ENABLE_PYTHON_JPM
PERSPECTIVE_EXPORT t_dtype get_dtype_from_numpy(t_uindex ndtype);
t_dtype get_dtype_from_numpy(PyArrayObject* arr);
t_int32 get_numpy_typenum_from_dtype(t_dtype dtype);
t_int32 get_numpy_typenum_from_dtype(t_dtype dtype);
#endif

#ifdef PSP_ENABLE_PYTHON
np::dtype get_numpy_typenum_from_dtype(t_dtype dtype);
#endif

} // end namespace perspective

#endif
