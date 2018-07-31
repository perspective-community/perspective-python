
#ifdef PSP_ENABLE_PYTHON
#include <iostream>
#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numpy.hpp>
#include <perspective/base.h>
#include <perspective/table.h>

#ifndef __PSP_BINDING_HPP__
#define __PSP_BINDING_HPP__

using namespace boost::python;
namespace np = boost::python::numpy;

void test(const char* name);


perspective::t_schema* t_schema_init(list& columns, list& types);

template<typename T>
void
_fill_col(std::vector<T> dcol, perspective::t_col_sptr col);

void
_fill_data_single_column(perspective::t_table& tbl,
                         const std::string& colname_i,
                         list& data_cols_i,
                         perspective::t_dtype col_type);

BOOST_PYTHON_MODULE(libbinding)
{
    def("test", test);

    enum_<perspective::t_dtype>("t_dtype")
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



    class_<perspective::t_schema>("t_schema",
        init<perspective::t_svec, perspective::t_dtypevec>())
        .def(init<>())
        .def(init<perspective::t_schema_recipe>())

        // custom constructor wrapper to make from python lists
        .def("__init__", make_constructor(&t_schema_init))

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
        .def("columns", &perspective::t_schema::columns, return_value_policy<copy_const_reference>())
        // .def("types", &perspective::t_schema::types, return_value_policy<copy_const_reference>())
    ;




    // need boost:noncopyable for PSP_NON_COPYABLE
    class_<perspective::t_table, boost::noncopyable>("t_table", init<perspective::t_schema>())

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
        .def("name", &perspective::t_table::name, return_value_policy<copy_const_reference>())
        .def("get_schema", &perspective::t_table::get_schema, return_value_policy<copy_const_reference>())

        // when multiple overloading methods, need to static_cast to specify
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)() const> (&perspective::t_table::num_rows))
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)(const perspective::t_mask&) const> (&perspective::t_table::num_rows))
        
        .def("pprint", static_cast<void (perspective::t_table::*)() const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(perspective::t_uindex, std::ostream*) const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(const perspective::t_str&) const>(&perspective::t_table::pprint))
        .def("pprint", static_cast<void (perspective::t_table::*)(const perspective::t_uidxvec&) const>(&perspective::t_table::pprint))


        // custom add ins
        .def("load_column", _fill_data_single_column)
    ;
}


#endif

#endif