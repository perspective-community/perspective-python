#include <iostream>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

#ifndef __PSP_BINDING_HPP__
#define __PSP_BINDING_HPP__

void test(const char* name);


BOOST_PYTHON_MODULE(test)
{
    boost::python::def("test", test);
}
#endif