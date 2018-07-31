
#ifdef PSP_ENABLE_PYTHON
#include <iostream>

#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/object.hpp>
#include <boost/python/converter/implicit.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/python/module.hpp>

#include <numpy/arrayobject.h>

#include <perspective/base.h>
#include <perspective/table.h>
 

#ifndef __PSP_BINDING_HPP__
#define __PSP_BINDING_HPP__

namespace py = boost::python;
namespace np = boost::python::numpy;


namespace
{
  inline NPY_TYPES get_typenum(bool) { return NPY_BOOL; }
  // inline NPY_TYPES get_typenum(npy_bool) { return NPY_BOOL; }
  inline NPY_TYPES get_typenum(npy_byte) { return NPY_BYTE; }
  inline NPY_TYPES get_typenum(npy_ubyte) { return NPY_UBYTE; }
  inline NPY_TYPES get_typenum(npy_short) { return NPY_SHORT; }
  inline NPY_TYPES get_typenum(npy_ushort) { return NPY_USHORT; }
  inline NPY_TYPES get_typenum(npy_int) { return NPY_INT; }
  inline NPY_TYPES get_typenum(npy_uint) { return NPY_UINT; }
  inline NPY_TYPES get_typenum(npy_long) { return NPY_LONG; }
  inline NPY_TYPES get_typenum(npy_ulong) { return NPY_ULONG; }
  inline NPY_TYPES get_typenum(npy_longlong) { return NPY_LONGLONG; }
  inline NPY_TYPES get_typenum(npy_ulonglong) { return NPY_ULONGLONG; }
  inline NPY_TYPES get_typenum(npy_float) { return NPY_FLOAT; }
  inline NPY_TYPES get_typenum(npy_double) { return NPY_DOUBLE; }
  inline NPY_TYPES get_typenum(npy_cfloat) { return NPY_CFLOAT; }
  inline NPY_TYPES get_typenum(npy_cdouble) { return NPY_CDOUBLE; }
  inline NPY_TYPES get_typenum(std::complex<float>) { return NPY_CFLOAT; }
  inline NPY_TYPES get_typenum(std::complex<double>) { return NPY_CDOUBLE; }
#if HAVE_LONG_DOUBLE && (NPY_SIZEOF_LONGDOUBLE > NPY_SIZEOF_DOUBLE)
  inline NPY_TYPES get_typenum(npy_longdouble) { return NPY_LONGDOUBLE; }
  inline NPY_TYPES get_typenum(npy_clongdouble) { return NPY_CLONGDOUBLE; }
  inline NPY_TYPES get_typenum(std::complex<long double>) { return NPY_CLONGDOUBLE; }
#endif
  inline NPY_TYPES get_typenum(boost::python::object) { return NPY_OBJECT; }
  inline NPY_TYPES get_typenum(boost::python::handle<>) { return NPY_OBJECT; }
 
  // array scalars ------------------------------------------------------------
  template <class T>
  const PyTypeObject *get_array_scalar_typeobj()
  {
    return (PyTypeObject *) PyArray_TypeObjectFromType(get_typenum(T()));
  }
 
  template <class T>
  void *check_array_scalar(PyObject *obj)
  {
    if (obj->ob_type == get_array_scalar_typeobj<T>())
      return obj;
    else
      return 0;
  }
 
  template <class T>
  static void convert_array_scalar(
      PyObject* obj,
      py::converter::rvalue_from_python_stage1_data* data)
  {
    void* storage = ((py::converter::rvalue_from_python_storage<T>*)data)->storage.bytes;
 
    // no constructor needed, only dealing with POD types
    PyArray_ScalarAsCtype(obj, reinterpret_cast<T *>(storage));
 
    // record successful construction
    data->convertible = storage;
  }
 
  // main exposer -------------------------------------------------------------
  template <class T>
  void expose_converters()
  {
    // conversion of array scalars
    py::converter::registry::push_back(
        check_array_scalar<T>
        , convert_array_scalar<T>
        , py::type_id<T>()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        , get_array_scalar_typeobj<T>
#endif
        );
  }
 
}
 
void pyublas_expose_converters()
{
  expose_converters<bool>();
  expose_converters<npy_byte>();
  expose_converters<npy_ubyte>();
  expose_converters<npy_short>();
  expose_converters<npy_ushort>();
  expose_converters<npy_int>();
  expose_converters<npy_uint>();
  expose_converters<npy_long>();
  expose_converters<npy_ulong>();
  expose_converters<npy_longlong>();
  expose_converters<npy_ulonglong>();
  expose_converters<npy_float>();
  expose_converters<npy_double>();
  expose_converters<std::complex<float> >();
  expose_converters<std::complex<double> >();
#if HAVE_LONG_DOUBLE            // defined in pyconfig.h
  expose_converters<npy_longdouble>();
  expose_converters<std::complex<long double> >();
#endif
}


void test(const char* name);


perspective::t_schema* t_schema_init(py::list& columns, py::list& types);

template<typename T>
void
_fill_col(std::vector<T> dcol, perspective::t_col_sptr col);

void
_fill_data_single_column(perspective::t_table& tbl,
                         const std::string& colname_i,
                         py::list& data_cols_i,
                         perspective::t_dtype col_type);

void
_fill_data_single_column(perspective::t_table& tbl,
                         const std::string& colname_i,
                         np::ndarray& data_cols_i,
                         perspective::t_dtype col_type);

BOOST_PYTHON_MODULE(libbinding)
{
    np::initialize();
    // import_array();
    pyublas_expose_converters();

    py::def("test", test);

    py::enum_<perspective::t_dtype>("t_dtype")
        .value("NONE", perspective::DTYPE_NONE)
        .value("INT64", perspective::DTYPE_INT64)
        .value("INT32", perspective::DTYPE_INT32)
        .value("INT16", perspective::DTYPE_INT16)
        .value("INT8", perspective::DTYPE_INT8)
        .value("UINT64", perspective::DTYPE_UINT64)
        .value("UINT32", perspective::DTYPE_UINT32)
        .value("UINT16", perspective::DTYPE_UINT16)
        .value("UINT8", perspective::DTYPE_UINT8)
        .value("FLOAT64", perspective::DTYPE_FLOAT64)
        .value("FLOAT32", perspective::DTYPE_FLOAT32)
        .value("BOOL", perspective::DTYPE_BOOL)
        .value("TIME", perspective::DTYPE_TIME)
        .value("DATE", perspective::DTYPE_DATE)
        .value("ENUM", perspective::DTYPE_ENUM)
        .value("OID", perspective::DTYPE_OID)
        .value("PTR", perspective::DTYPE_PTR)
        .value("F64PAIR", perspective::DTYPE_F64PAIR)
        .value("USER_FIXED", perspective::DTYPE_USER_FIXED)
        .value("STR", perspective::DTYPE_STR)
        .value("NP_INT64", perspective::DTYPE_NP_INT64)
        .value("NP_FLOAT64", perspective::DTYPE_NP_FLOAT64)
        .value("NP_STR", perspective::DTYPE_NP_STR)
        .value("NP_BOOL", perspective::DTYPE_NP_BOOL)
        .value("NP_COMPLEX128", perspective::DTYPE_NP_COMPLEX128)
        .value("NP_DATE", perspective::DTYPE_NP_DATE)
        .value("NP_TIME", perspective::DTYPE_NP_TIME)
        .value("USER_VLEN", perspective::DTYPE_USER_VLEN)
        .value("LAST_VLEN", perspective::DTYPE_LAST_VLEN)
        .value("LAST", perspective::DTYPE_LAST)
    ;



    py::class_<perspective::t_schema>("t_schema",
        py::init<perspective::t_svec, perspective::t_dtypevec>())
        .def(py::init<>())
        .def(py::init<perspective::t_schema_recipe>())

        // custom constructor wrapper to make from python lists
        .def("__init__", py::make_constructor(&t_schema_init))

        // regular methods
        .def("get_num_columns", &perspective::t_schema::get_num_columns)
        .def("size", &perspective::t_schema::size)
        .def("get_colidx", &perspective::t_schema::get_colidx)
        .def("get_dtype", &perspective::t_schema::get_dtype)
        .def("is_pkey", &perspective::t_schema::is_pkey)
        .def("add_column", &perspective::t_schema::add_column)
        .def("get_recipe", &perspective::t_schema::get_recipe)
        .def("has_column", &perspective::t_schema::has_column)
        .def("get_table_context", &perspective::t_schema::get_table_context)

        // when returning const, need return_value_policy<copy_const_reference>
        .def("columns", &perspective::t_schema::columns, py::return_value_policy<py::copy_const_reference>())
        // .def("types", &perspective::t_schema::types, return_value_policy<copy_const_reference>())
    ;




    // need boost:noncopyable for PSP_NON_COPYABLE
    py::class_<perspective::t_table, boost::noncopyable>("t_table", py::init<perspective::t_schema>())

        .def("init", &perspective::t_table::init)
        .def("clear", &perspective::t_table::clear)
        .def("reset", &perspective::t_table::reset)
        .def("size", &perspective::t_table::size)
        .def("reserve", &perspective::t_table::reserve)
        .def("extend", &perspective::t_table::extend)
        .def("set_size", &perspective::t_table::set_size)

        .def("num_columns", &perspective::t_table::num_columns)
        .def("get_capacity", &perspective::t_table::get_capacity)

        // when returning const, need return_value_policy<copy_const_reference>
        .def("name", &perspective::t_table::name, py::return_value_policy<py::copy_const_reference>())
        .def("get_schema", &perspective::t_table::get_schema, py::return_value_policy<py::copy_const_reference>())

        // when multiple overloading methods, need to static_cast to specify
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)() const> (&perspective::t_table::num_rows))
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)(const perspective::t_mask&) const> (&perspective::t_table::num_rows))
        
        .def("pprint", static_cast<void (perspective::t_table::*)() const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(perspective::t_uindex, std::ostream*) const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(const perspective::t_str&) const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(const perspective::t_uidxvec&) const>(&perspective::t_table::pprint))


        // custom add ins
        // .def("load_column", _fill_data_single_column)
        .def("load_column", static_cast<void (*)(perspective::t_table& tbl, const std::string& colname_i, py::list& data_cols_i, perspective::t_dtype col_type)>(_fill_data_single_column))
        .def("load_column", static_cast<void (*)(perspective::t_table& tbl, const std::string& colname_i, np::ndarray& data_cols_i, perspective::t_dtype col_type)>(_fill_data_single_column))
    ;
}


#endif

#endif