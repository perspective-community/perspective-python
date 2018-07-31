#ifdef PSP_ENABLE_PYTHON
#include <perspective/binding.hpp>

void test(const char* name) {
    std::cout << "Hello " <<  name << "!" << std::endl;
}

perspective::t_schema* t_schema_init(list& columns, list& types)
{
    perspective::t_svec cols;
    perspective::t_dtypevec ts;

    for(ssize_t i=0; i < len(columns); i++) {
        cols.push_back(extract<perspective::t_str>(columns[i]));
    }

    for(ssize_t i=0; i < len(types); i++) {
        ts.push_back(extract<perspective::t_dtype>(types[i]));
    }

    return new perspective::t_schema(cols, ts);
}

template<typename T>
void
_fill_col(std::vector<T> dcol, perspective::t_col_sptr col)
{
    perspective::t_uindex nrows = col->size();

    for (auto i = 0; i < nrows; ++i)
    {
        auto elem = dcol[i];
        col->set_nth(i, elem);
    }
}

void
_fill_data_single_column(perspective::t_table& tbl,
                         const std::string& colname_i,
                         list& data_cols_i,
                         perspective::t_dtype col_type)
{
    perspective::t_str name = colname_i;
    perspective::t_col_sptr col = tbl.get_column(name);

    switch(col_type){
        case perspective::DTYPE_INT64 : {
            std::vector<perspective::t_int64> dcol;

            for(ssize_t i=0; i < len(data_cols_i); i++)
            {
                dcol.push_back(extract<perspective::t_int64>(data_cols_i[i]));
            }

            _fill_col<perspective::t_int64>(dcol, col);
            break;
        }
        case perspective::DTYPE_UINT64 : {
            std::vector<perspective::t_uint64> dcol;

            for(ssize_t i=0; i < len(data_cols_i); i++)
            {
                dcol.push_back(extract<perspective::t_uint64>(data_cols_i[i]));
            }

            _fill_col<perspective::t_uint64>(dcol, col);
            break;
        }
        case perspective::DTYPE_FLOAT64 : {
            std::vector<perspective::t_float64> dcol;

            for(ssize_t i=0; i < len(data_cols_i); i++)
            {
                dcol.push_back(extract<perspective::t_float64>(data_cols_i[i]));
            }

            _fill_col<perspective::t_float64>(dcol, col);
            break;
        }
        case perspective::DTYPE_BOOL : {
            //FIXME segfault
            std::vector<perspective::t_bool> dcol;

            for(ssize_t i=0; i < len(data_cols_i); i++)
            {
                dcol.push_back(extract<perspective::t_bool>(data_cols_i[i]));
            }

            _fill_col<perspective::t_bool>(dcol, col);
            break;
        }
        case perspective::DTYPE_STR : {

            std::vector<perspective::t_str> dcol;

            for(ssize_t i=0; i < len(data_cols_i); i++)
            {
                dcol.push_back(extract<perspective::t_str>(data_cols_i[i]));
            }

            _fill_col<perspective::t_str>(dcol, col);
            break;
        }
#ifdef PSP_ENABLE_PYTHON
        case perspective::DTYPE_NP_INT64 : {
            break;
        }
        case perspective::DTYPE_NP_FLOAT64 : {
            break;
        }
        case perspective::DTYPE_NP_STR : {
            break;
        }
        case perspective::DTYPE_NP_COMPLEX128 : {
            break;
        }
        case perspective::DTYPE_NP_DATE : {
            break;
        }
        case perspective::DTYPE_NP_TIME : {
            break;
        }
#endif  
        default: {
            break;
        }
    }
}


#endif

