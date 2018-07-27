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

    // class_<perspective::t_table>("t_table", init<perspective::t_schema>());
    //     .def("num_columns", &perspective::t_table::num_columns)
    //     .def("num_rows", &perspective::t_table::num_rows)
    //     .def("size", &perspective::t_table::size)
    //     .def("get_schema", &perspective::t_table::get_schema)
    //     .def("get_capacity", &perspective::t_table::get_capacity)
    //     .def("pprint", &perspective::t_table::pprint)
    // ;
}
#endif