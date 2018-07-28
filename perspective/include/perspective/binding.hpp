#include <iostream>
#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <perspective/base.h>
#include <perspective/table.h>

#ifndef __PSP_BINDING_HPP__
#define __PSP_BINDING_HPP__

using namespace boost::python;

void test(const char* name);

BOOST_PYTHON_MODULE(libbinding)
{
    def("test", test);

    class_<perspective::t_schema>("t_schema", init<perspective::t_svec, perspective::t_dtypevec>());

    // need boost:noncopyable for PSP_NON_COPYABLE
    class_<perspective::t_table, boost::noncopyable>("t_table", init<perspective::t_schema>())

        .def("size", &perspective::t_table::size)
        .def("num_columns", &perspective::t_table::num_columns)

        // when returning const, need return_value_policy<copy_const_reference>
        .def("name", &perspective::t_table::name, return_value_policy<copy_const_reference>())
        .def("get_schema", &perspective::t_table::get_schema, return_value_policy<copy_const_reference>())

        // when multiple overloading methods, need to static_cast to specify
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)() const> (&perspective::t_table::num_rows))
        .def("num_rows", static_cast<perspective::t_uindex (perspective::t_table::*)(const perspective::t_mask&) const> (&perspective::t_table::num_rows));


        // .def("pprint", &perspective::t_table::pprint);
        // .def("get_capacity", &perspective::t_table::get_capacity)

}
#endif