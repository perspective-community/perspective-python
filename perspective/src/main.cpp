/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <vector>
#include <string>
#include <perspective/base.h>
#include <perspective/gnode.h>
#include <perspective/table.h>
#include <perspective/pool.h>
#include <perspective/context_zero.h>
#include <perspective/context_one.h>
#include <perspective/context_two.h>
#include <random>
#include <cmath>
#include <sstream>
#include <perspective/sym_table.h>

using namespace perspective;


/**
 * Main
 */
int
main(int argc, char** argv)
{
    t_svec colnames = std::vector<std::string>();
    t_dtypevec dtypes = std::vector<t_dtype>();

    t_uint32 size = 1000;

    auto tbl = std::make_shared<t_table>(t_schema(colnames, dtypes));
    tbl->init();
    tbl->extend(size);

    std::cout << "Perspective initialized successfully." << std::endl;
}
