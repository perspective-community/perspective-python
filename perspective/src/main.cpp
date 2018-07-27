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
#include <perspective/table.h>
#include <random>
#include <cmath>
#include <sstream>

using namespace perspective;

template<typename T>
void
_fill_col(std::vector<T> dcol, t_col_sptr col)
{
    t_uindex nrows = col->size();

    for (auto i = 0; i < nrows; ++i)
    {
        auto elem = dcol[i];
        col->set_nth(i, elem);
    }
}

template<typename T>
void
_fill_data(t_table_sptr tbl,
           t_svec ocolnames,
           std::vector<std::vector<T>> data_cols,
           std::vector<t_dtype> odt,
           t_uint32 offset)
{
    for (auto cidx = 0; cidx < ocolnames.size(); ++cidx)
    {
        auto name = ocolnames[cidx];
        auto col = tbl->get_column(name);
        auto col_type = odt[cidx];
        auto dcol = data_cols[cidx];
        _fill_col<T>(dcol, col);
    }
}


/**
 * Main
 */
int
main(int argc, char** argv)
{
    t_svec colnames = {"Col1", "Col2"};
    t_dtypevec dtypes = {DTYPE_INT64, DTYPE_STR};

    t_uint32 size = 10;

    auto tbl = std::make_shared<t_table>(t_schema(colnames, dtypes));
    tbl->init();
    tbl->extend(size);

    t_svec colnames1 = {"Col1"};
    std::vector<std::vector<t_int64> > data1 = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
    t_dtypevec dtype1 = {DTYPE_INT64};
    _fill_data<t_int64>(tbl, colnames1, data1, dtype1, 0);


    t_svec colnames2 = {"Col2"};
    std::vector<std::vector<std::string> > data2 = {{"test0", "test1", "test2", "test3", "test4", "test5", "test6", "test7", "test8", "test9",}};
    t_dtypevec dtype2 = {DTYPE_STR};
    _fill_data<std::string>(tbl, colnames2, data2, dtype2, 0);

    std::cout << "Perspective initialized successfully." << std::endl;
    std::cout << tbl->num_columns() << std::endl;
    std::cout << tbl->num_rows() << std::endl;
    std::cout << tbl->size() << std::endl;
    std::cout << tbl->get_schema() << std::endl;
    std::cout << tbl->get_capacity() << std::endl;
    tbl->pprint();
}
