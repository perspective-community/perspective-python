/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#pragma once
#include <perspective/first.h>
#include <perspective/base.h>
#include <perspective/column.h>
#include <perspective/schema.h>
#include <perspective/schema_column.h>
#include <perspective/exports.h>
#include <perspective/mask.h>
#include <perspective/filter.h>
#include <perspective/compat.h>

#ifdef PSP_ENABLE_PYTHON_JPM
#include <polaris/jitcompiler_psp.h>
#endif

#ifdef PSP_PARALLEL_FOR
#include <tbb/parallel_sort.h>
#include <tbb/tbb.h>
#endif

#include <tuple>

namespace perspective
{

template <typename DATA_T>
struct t_rowpack
{
    DATA_T m_pkey;
    t_index m_idx;
    t_op m_op;
};

struct t_flatten_record
{
    t_uindex m_store_idx;
    t_uindex m_bidx;
    t_uindex m_eidx;
};

class t_table;
typedef std::vector<t_table> t_tblvec;
typedef std::shared_ptr<t_table> t_table_sptr;
typedef std::shared_ptr<const t_table> t_table_csptr;

enum t_table_mode
{
    TABLE_MODE_FREE_STANDING_DENSE,
    TABLE_MODE_FREE_STANDING_SPARSE,
    TABLE_MODE_SUBSET_RANGE,
    TABLE_MODE_SUBSET_MASKED,
    TABLE_MODE_SUBSET_KEYED
};

struct PERSPECTIVE_EXPORT t_table_recipe
{
    t_table_recipe();

    // TODO
    void release_storage();

    t_str m_name;
    t_str m_dirname;
    t_schema_recipe m_schema;
    t_uindex m_size;
    t_uindex m_capacity;
    t_backing_store m_backing_store;
    // in same order as m_schema.m_columns
    t_column_recipe_vec m_columns;
};

#ifdef PSP_ENABLE_PYTHON_JPM
typedef JITCompiler::PspCompiledComputedColumn t_jit;
typedef std::shared_ptr< t_jit > t_jitsptr;

struct PERSPECTIVE_EXPORT t_expr_info
{
    t_expr_info(t_jitsptr jit, const t_str& ocol);

    t_jitsptr m_jit;
    t_str m_ocol;
};

typedef std::vector<t_expr_info> t_expr_infovec;
#endif

class PERSPECTIVE_EXPORT t_table
{
    friend class t_jit_ctx;

  public:
#ifdef PSP_DBG_MALLOC
    PSP_NEW_DELETE(t_table)
#endif
    PSP_NON_COPYABLE(t_table);
    t_table(const t_table_recipe& recipe);
    t_table(const t_schema& s,
            t_uindex capacity = DEFAULT_EMPTY_CAPACITY);
    t_table(const t_str& name,
            const t_str& dirname,
            const t_schema& s,
            t_uindex init_cap,
            t_backing_store backing_store);
    ~t_table();

    const t_str& name() const;
    void init();
    t_uindex num_columns() const;
    t_uindex num_rows() const;
    t_uindex num_rows(const t_mask& q) const;
    const t_schema& get_schema() const;
    t_uindex size() const;
    t_uindex get_capacity() const;
    t_dtype get_dtype(const t_str& colname) const;

    t_col_sptr get_column(const t_str& colname);
    t_col_csptr get_const_column(const t_str& colname) const;

    t_col_sptr get_column(t_uindex idx);
    t_col_csptr get_const_column(t_uindex idx) const;

    // Only increment capacity
    void reserve(t_uindex nelems);

    // Increment capacity and size
    void extend(t_uindex nelems);

    void set_size(t_uindex size);

    t_column* _get_column(const t_str& colname);

    t_table_sptr flatten() const;

    t_bool is_pkey_table() const;
    t_bool is_same_shape(t_table& tbl) const;

    t_table_sptr empty_like() const;

    void pprint() const;
    void pprint(t_uindex nrows, std::ostream* os = 0) const;
    void pprint(const t_str& fname) const;
    void pprint(const t_uidxvec& vec) const;
    void pprint_vocabulary() const;

    void append(const t_table& other);

    void clear();
    void reset();

    t_mask filter_cpp(t_filter_op combiner,
                      const t_ftermvec& fops) const;
    t_table* clone_(const t_mask& mask) const;
    t_table_sptr clone(const t_mask& mask) const;

    t_column* clone_column(const t_str& existing_col,
                           const t_str& new_colname);
#ifdef PSP_ENABLE_PYTHON_JPM
    PyObject* filter(t_filter_op combiner,
                     const t_ftermvec& fops) const;
    void fill_expr_helper(const t_svec& icol_names,
                          const t_str& expr,
                          const t_str& output_column,
                          const t_svec& where_keys,
                          const t_svec& where_values,
                          const t_str& base_case);
    void fill_expr(const t_str& expr, const t_str& output_column);
    void fill_expr(const t_svec& icol_names,
                   const t_str& expr,
                   const t_str& output_column,
                   const t_svec& where_keys,
                   const t_svec& where_values,
                   const t_str& base_case);
    void fill_expr_jit(PyObject* fn, const t_str& output_column);
    void fill_expr_jit(t_uindex expidx);
    t_uindex register_jit_expr(PyObject* fn,
                               const t_str& output_column);
    t_mask where(const t_str& expr) const;
#endif

    t_table_recipe get_recipe() const;

    t_colcptrvec get_const_columns() const;
    t_colptrvec get_columns();

    void set_column(t_uindex idx, t_col_sptr col);
    void set_column(const t_str& name, t_col_sptr col);

    void join_inplace(const t_svec& on,
                      const t_svec& update_cols,
                      const t_table& other);

    // return the ops required to make
    t_table diff(const t_table& tbl) const;

    t_table* _flatten() const;

    t_column* add_column(const t_str& cname,
                         t_dtype dtype,
                         t_bool valid_enabled);

    t_col_sptr make_column(const t_str& colname,
                           t_dtype dtype,
                           t_bool valid_enabled);

    t_table* project(const t_svec& columns) const;
    t_table* select(const t_mask& mask) const;
    void verify() const;
    t_tscalvec _as_list() const;
    void set_capacity(t_uindex idx);

    template <typename TPL_T, std::size_t remaining>
    struct set_row_helper
    {
        static void
        fn(t_table* tbl, t_uindex ridx, const TPL_T& t)
        {
            const int cidx =
                std::tuple_size<TPL_T>::value - remaining;
            typedef
                typename std::tuple_element<cidx, TPL_T>::type elem_t;
            tbl->m_columns[cidx]->set_nth<elem_t>(ridx,
                                                  std::get<cidx>(t));
            set_row_helper<TPL_T, remaining - 1>::fn(tbl, ridx, t);
        }
    };

    template <typename TPL_T>
    struct set_row_helper<TPL_T, 0>
    {
        static void
        fn(t_table* tbl, int ridx, const TPL_T& t)
        {
        }
    };

    template <typename TPL_T>
    void
    set_row(t_uindex ridx, const TPL_T& t)
    {
        set_row_helper<TPL_T, std::tuple_size<TPL_T>::value>::fn(
            this, ridx, t);
    }

  protected:
    template <typename FLATTENED_T>
    void flatten_body(FLATTENED_T flattened) const;

    template <typename FLATTENED_T, typename PKEY_T>
    void flatten_helper_1(FLATTENED_T flattened) const;

    template <typename DATA_T, typename ROWPACK_VEC_T>
    void flatten_helper_2(ROWPACK_VEC_T& sorted,
                          std::vector<t_flatten_record>& fltrecs,
                          const t_column* scol,
                          t_column* dcol) const;

    void flatten_common(const t_tscalvec& row,
                        t_uindex ncols,
                        t_table_sptr tbl,
                        std::vector<t_column*>& columns) const;
    t_str repr() const;

    t_column_dynamic_ctxvec get_dynamic_context() const;

  private:
    t_str m_name;
    t_str m_dirname;
    t_schema m_schema;
    t_uindex m_size;
    t_uindex m_capacity;
    t_backing_store m_backing_store;
    t_bool m_init;
    t_colsptrvec m_columns;
    t_table_recipe m_recipe;
    t_bool m_from_recipe;
#ifdef PSP_ENABLE_PYTHON_JPM
    t_expr_infovec m_einfovec;
#endif
};

template <typename FLATTENED_T>
void
t_table::flatten_body(FLATTENED_T flattened) const
{
    PSP_TRACE_SENTINEL();
    PSP_VERBOSE_ASSERT(m_init, "touching uninited object");
    PSP_VERBOSE_ASSERT(is_pkey_table(), "Not a pkeyed table");

    switch (get_const_column("psp_pkey")->get_dtype())
    {
        case DTYPE_INT64:
        {
            flatten_helper_1<FLATTENED_T, t_int64>(flattened);
        }
        break;
        case DTYPE_INT32:
        {
            flatten_helper_1<FLATTENED_T, t_int32>(flattened);
        }
        break;
        case DTYPE_INT16:
        {
            flatten_helper_1<FLATTENED_T, t_int16>(flattened);
        }
        break;
        case DTYPE_INT8:
        {
            flatten_helper_1<FLATTENED_T, t_int8>(flattened);
        }
        break;
        case DTYPE_UINT64:
        {
            flatten_helper_1<FLATTENED_T, t_uint64>(flattened);
        }
        break;
        case DTYPE_UINT32:
        {
            flatten_helper_1<FLATTENED_T, t_uint32>(flattened);
        }
        break;
        case DTYPE_UINT16:
        {
            flatten_helper_1<FLATTENED_T, t_uint16>(flattened);
        }
        break;
        case DTYPE_UINT8:
        {
            flatten_helper_1<FLATTENED_T, t_uint8>(flattened);
        }
        break;
        case DTYPE_TIME:
        {
            flatten_helper_1<FLATTENED_T, t_int64>(flattened);
        }
        break;
        case DTYPE_DATE:
        {
            flatten_helper_1<FLATTENED_T, t_uint32>(flattened);
        }
        break;
        case DTYPE_STR:
        {
            flatten_helper_1<FLATTENED_T, t_stridx>(flattened);
        }
        break;
        case DTYPE_FLOAT64:
        {
            flatten_helper_1<FLATTENED_T, t_float64>(flattened);
        }
        break;
        case DTYPE_FLOAT32:
        {
            flatten_helper_1<FLATTENED_T, t_float32>(flattened);
        }
        break;
        default:
        {
            PSP_COMPLAIN_AND_ABORT("Unsupported key type");
        }
    }

    return;
}

template <typename DATA_T, typename ROWPACK_VEC_T>
void
t_table::flatten_helper_2(ROWPACK_VEC_T& sorted,
                          std::vector<t_flatten_record>& fltrecs,
                          const t_column* scol,
                          t_column* dcol) const
{
    for (const auto& rec : fltrecs)
    {
        t_bool added = false;
        t_index fragidx = 0;
        for (t_index spanidx = rec.m_eidx - 1;
             spanidx >= t_index(rec.m_bidx);
             --spanidx)
        {
            const auto& sort_rec = sorted[spanidx];
            fragidx = sort_rec.m_idx;
            if (scol->is_valid(fragidx))
            {
                added = true;
                break;
            }
        }

        if (added)
        {
            dcol->set_nth<DATA_T>(rec.m_store_idx,
                                  *(scol->get_nth<DATA_T>(fragidx)),
                                  true);
        }
    }
}

template <typename FLATTENED_T, typename PKEY_T>
void
t_table::flatten_helper_1(FLATTENED_T flattened) const
{
    t_uindex frags_size = size();

    PSP_VERBOSE_ASSERT(is_same_shape(*flattened),
                       "Misaligned shaped found");

    if (frags_size == 0)
        return;

    t_colcptrvec s_columns;
    t_colptrvec d_columns;

    for (const auto& colname : m_schema.m_columns)
    {
        if (colname != "psp_pkey" && colname != "psp_op")
        {
            s_columns.push_back(get_const_column(colname).get());
            d_columns.push_back(flattened->get_column(colname).get());
        }
    }

    const t_column* s_pkey_col = get_const_column("psp_pkey").get();
    const t_column* s_op_col = get_const_column("psp_op").get();

    t_column* d_pkey_col = flattened->get_column("psp_pkey").get();
    t_column* d_op_col = flattened->get_column("psp_op").get();

    typedef std::vector<t_rowpack<PKEY_T>> t_rpvec;

    std::vector<t_rowpack<PKEY_T>> sorted(frags_size);
    for (t_uindex fragidx = 0; fragidx < frags_size; ++fragidx)
    {
        sorted[fragidx].m_pkey =
            *(s_pkey_col->get_nth<PKEY_T>(fragidx));
        sorted[fragidx].m_op =
            static_cast<t_op>(*(s_op_col->get_nth<t_uint8>(fragidx)));
        sorted[fragidx].m_idx = fragidx;
    }

    struct t_packcomp
    {
        bool
        operator()(const t_rowpack<PKEY_T>& a,
                   const t_rowpack<PKEY_T>& b) const
        {
            return a.m_pkey < b.m_pkey ||
                   (!(b.m_pkey < a.m_pkey) && a.m_idx < b.m_idx);
        }
    };

    t_packcomp cmp;
    std::sort(sorted.begin(), sorted.end(), cmp);

    t_idxvec edges;
    edges.push_back(0);

    for (t_index idx = 1, loop_end = sorted.size(); idx < loop_end;
         ++idx)
    {
        if (sorted[idx].m_pkey != sorted[idx - 1].m_pkey)
        {
            edges.push_back(idx);
        }
    }

    flattened->reserve(size());

    std::vector<t_flatten_record> fltrecs;

    t_uindex store_idx = 0;

    for (t_index fidx = 0, loop_end = edges.size(); fidx < loop_end;
         ++fidx)
    {
        t_index bidx = edges[fidx];
        t_bool edge_bool =
            fidx == static_cast<t_index>(edges.size() - 1);
        t_index eidx = edge_bool ? sorted.size() : edges[fidx + 1];

        bool delete_encountered = false;

        for (t_index spanidx = bidx; spanidx < eidx; ++spanidx)
        {
            if (sorted[spanidx].m_op == OP_DELETE)
            {
                bidx = spanidx;
                delete_encountered = true;
            }
        }

        const auto& sort_rec = sorted[bidx];

        if (delete_encountered)
        {
            d_pkey_col->push_back(sort_rec.m_pkey);
            t_uint8 op8 = OP_DELETE;
            d_op_col->push_back(op8);
            ++store_idx;
        }

        if (!delete_encountered ||
            (delete_encountered && bidx + 1 != eidx))
        {
            t_flatten_record rec;
            rec.m_store_idx = store_idx;
            rec.m_bidx = bidx;
            rec.m_eidx = eidx;
            fltrecs.push_back(rec);

            d_pkey_col->push_back(sort_rec.m_pkey);

            t_uint8 op8 = OP_INSERT;
            d_op_col->push_back(op8);
            ++store_idx;
        }
    }

    flattened->set_size(store_idx);
    t_uindex ndata_cols = d_columns.size();

#ifdef PSP_PARALLEL_FOR
    PSP_PFOR(
        0,
        int(ndata_cols),
        1,
        [&s_columns, &sorted, &d_columns, &fltrecs, this](int colidx)
#else
    for (t_uindex colidx = 0; colidx < ndata_cols; ++colidx)
#endif
        {

            auto scol = s_columns[colidx];
            auto dcol = d_columns[colidx];

            switch (scol->get_dtype())
            {
                case DTYPE_INT64:
                {
                    this->flatten_helper_2<t_int64, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_INT32:
                {
                    this->flatten_helper_2<t_int32, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_INT16:
                {
                    this->flatten_helper_2<t_int16, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_INT8:
                {
                    this->flatten_helper_2<t_int8, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_UINT64:
                {
                    this->flatten_helper_2<t_uint64, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_UINT32:
                {
                    this->flatten_helper_2<t_uint32, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_UINT16:
                {
                    this->flatten_helper_2<t_uint16, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_UINT8:
                {
                    this->flatten_helper_2<t_uint8, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_FLOAT64:
                {
                    this->flatten_helper_2<t_float64, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_FLOAT32:
                {
                    this->flatten_helper_2<t_float32, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_BOOL:
                {
                    this->flatten_helper_2<t_uint8, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_TIME:
                {
                    this->flatten_helper_2<t_int64, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_DATE:
                {
                    this->flatten_helper_2<t_uint32, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                case DTYPE_STR:
                {
                    this->flatten_helper_2<t_stridx, t_rpvec>(
                        sorted, fltrecs, scol, dcol);
                }
                break;
                default:
                {
                    PSP_COMPLAIN_AND_ABORT(
                        "Unsupported column dtype");
                }
            }
        }
#ifdef PSP_PARALLEL_FOR
        );
#endif

#ifdef PSP_PARALLEL_FOR
    PSP_PFOR(
        0,
        int(m_schema.get_num_columns()),
        [&flattened, this](int colidx)
#else
    for (t_uindex colidx = 0, loop_end = m_schema.get_num_columns();
         colidx < loop_end;
         ++colidx)
#endif
        {
            const auto& colname = this->m_schema.m_columns[colidx];
            auto col = get_const_column(colname).get();
            if (col->get_dtype() == DTYPE_STR)
            {
                flattened->get_column(colname)->copy_vocabulary(col);
            }

        }
#ifdef PSP_PARALLEL_FOR
        );
#endif

    d_pkey_col->valid_raw_fill(true);
    d_op_col->valid_raw_fill(true);
}

typedef std::shared_ptr<t_table> t_table_sptr;
typedef std::shared_ptr<const t_table> t_table_csptr;

typedef std::vector<t_table_sptr> t_tblsvec;
typedef std::vector<t_table_csptr> t_tblcsvec;

} // end namespace perspective
