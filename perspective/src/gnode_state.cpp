/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <perspective/context_one.h>
#include <perspective/context_two.h>
#include <perspective/context_zero.h>
#include <perspective/gnode_state.h>
#include <perspective/mask.h>
#include <perspective/sym_table.h>
#ifdef PSP_PARALLEL_FOR
#include <tbb/tbb.h>
#endif

namespace perspective
{

t_gstate::t_gstate(const t_schema& tblschema,
                   const t_schema& pkeyed_schema)

    : m_tblschema(tblschema), m_pkeyed_schema(pkeyed_schema),
      m_init(false)
{
    LOG_CONSTRUCTOR("t_gstate");
}

t_gstate::~t_gstate()
{
    LOG_DESTRUCTOR("t_gstate");
}

void
t_gstate::init()
{
    m_table = std::make_shared<t_table>("",
                                        "",
                                        m_pkeyed_schema,
                                        DEFAULT_EMPTY_CAPACITY,
                                        BACKING_STORE_MEMORY);
    m_table->init();
    m_pkcol = m_table->get_column("psp_pkey");
    m_opcol = m_table->get_column("psp_op");
    m_init = true;
}

t_rlookup
t_gstate::lookup(t_tscalar pkey) const
{
    t_rlookup rval(0, false);

    t_mapping::const_iterator iter = m_mapping.find(pkey);

    if (iter == m_mapping.end())
        return rval;

    rval.m_idx = iter->second;
    rval.m_exists = true;
    return rval;
}

void
t_gstate::_mark_deleted(t_uindex idx)
{
    m_free.insert(idx);
}

void
t_gstate::erase(const t_tscalar& pkey)
{
    t_mapping::const_iterator iter = m_mapping.find(pkey);

    if (iter == m_mapping.end())
    {
        return;
    }

    auto columns = m_table->get_columns();

    t_uindex idx = iter->second;

    for (auto c : columns)
    {
        c->clear(idx);
    }

    m_mapping.erase(iter);
    _mark_deleted(idx);
}

t_uindex
t_gstate::lookup_or_create(const t_tscalar& pkey)
{
    auto pkey_ = m_symtable.get_interned_tscalar(pkey);

    t_mapping::const_iterator iter = m_mapping.find(pkey_);

    if (iter != m_mapping.end())
    {
        return iter->second;
    }

    if (!m_free.empty())
    {
        t_free_items::const_iterator iter = m_free.begin();
        t_uindex idx = *iter;
        m_free.erase(iter);
        m_mapping[pkey_] = idx;
        return idx;
    }

    t_uindex nrows = m_table->num_rows();
    if (nrows >= m_table->get_capacity() - 1)
    {
        m_table->reserve(
            std::max(nrows + 1,
                     static_cast<t_uindex>(m_table->get_capacity() *
                                           PSP_TABLE_GROW_RATIO)));
    }

    m_table->set_size(nrows + 1);
    m_opcol->set_nth<t_uint8>(nrows, OP_INSERT);
    m_pkcol->set_scalar(nrows, pkey);
    m_mapping[pkey_] = nrows;
    return nrows;
}

void
t_gstate::update_history(const t_table* tbl)
{
    const t_schema& fschema = tbl->get_schema();
    const t_schema& sschema = m_table->get_schema();

    auto pkey_col = tbl->get_const_column("psp_pkey").get();
    auto op_col = tbl->get_const_column("psp_op").get();

    t_uindex ncols = m_table->num_columns();
    auto stable = m_table.get();

    t_uidxvec col_translation(stable->num_columns());

    t_str opname("psp_op");
    t_str pkeyname("psp_pkey");

    t_colcptrvec fcolumns(tbl->num_columns());

    t_uindex count = 0;
    for (t_uindex idx = 0, loop_end = fschema.size(); idx < loop_end;
         ++idx)
    {
        const t_str& cname = fschema.m_columns[idx];
            col_translation[count] = idx;
            fcolumns[idx] = tbl->get_const_column(cname).get();
            ++count;
        }

    t_colptrvec scolumns(ncols);

    for (t_uindex idx = 0, loop_end = sschema.size(); idx < loop_end;
         ++idx)
    {
        const t_str& cname = sschema.m_columns[idx];
        scolumns[idx] = stable->get_column(cname).get();
    }

    if (size() == 0)
    {
        m_free.clear();
        m_mapping.clear();
#ifdef PSP_PARALLEL_FOR
        PSP_PFOR(0,
                 int(ncols),
                 1,
                 [&stable, &fcolumns, &col_translation](int idx)
#else
        for (t_uindex idx = 0; idx < ncols; ++idx)
#endif
                 {
                     stable->set_column(
                         idx,
                         fcolumns[col_translation[idx]]->clone());
                 }
#ifdef PSP_PARALLEL_FOR
                 );
#endif
        m_pkcol = stable->get_column("psp_pkey");
        m_opcol = stable->get_column("psp_op");

        stable->set_capacity(tbl->get_capacity());
        stable->set_size(tbl->size());

        for (t_uindex idx = 0, loop_end = tbl->num_rows();
             idx < loop_end;
             ++idx)
        {
            t_tscalar pkey = pkey_col->get_scalar(idx);
            const t_uint8* op_ptr = op_col->get_nth<t_uint8>(idx);
            t_op op = static_cast<t_op>(*op_ptr);

            switch (op)
            {
                case OP_INSERT:
                {
                    m_mapping[m_symtable.get_interned_tscalar(pkey)] =
                        idx;
                    m_opcol->set_nth<t_uint8>(idx, OP_INSERT);
                    m_pkcol->set_scalar(idx, pkey);
                }
                break;
                case OP_DELETE:
                {
                    _mark_deleted(idx);
                }
                break;
                default:
                {
                    PSP_COMPLAIN_AND_ABORT("Unexpected OP");
                }
                break;
            }
        }

#ifdef PSP_TABLE_VERIFY
        stable->verify();
#endif

        return;
    }

    /* size is not zero */
    t_uidxvec stableidx_vec(tbl->num_rows());

    for (t_uindex idx = 0, loop_end = tbl->num_rows(); idx < loop_end;
         ++idx)
    {
        t_tscalar pkey = pkey_col->get_scalar(idx);
        const t_uint8* op_ptr = op_col->get_nth<t_uint8>(idx);
        t_op op = static_cast<t_op>(*op_ptr);

        switch (op)
        {
            case OP_INSERT:
            {
                stableidx_vec[idx] = lookup_or_create(pkey);
                m_opcol->set_nth<t_uint8>(stableidx_vec[idx],
                                          OP_INSERT);
                m_pkcol->set_scalar(stableidx_vec[idx], pkey);
            }
            break;
            case OP_DELETE:
            {
                erase(pkey);
            }
            break;
            default:
            {
                PSP_COMPLAIN_AND_ABORT("Unexpected OP");
            }
            break;
        }
    }

#ifdef PSP_PARALLEL_FOR
    const t_mapping* m = &m_mapping;
    PSP_PFOR(
        0,
        int(ncols),
        1,
        [m,
         tbl,
         pkey_col,
         op_col,
         &col_translation,
         &fcolumns,
         &scolumns,
         &stableidx_vec](int colidx)
#else
    for (t_uindex colidx = 0; colidx < ncols; ++colidx)
#endif

        {
            const t_column* fcolumn =
                fcolumns[col_translation[colidx]];

            t_column* scolumn = scolumns[colidx];

            for (t_uindex idx = 0, loop_end = tbl->num_rows();
                 idx < loop_end;
                 ++idx)
            {

                t_bool is_valid = fcolumn->is_valid(idx);

                if (!is_valid)
                    continue;

                const t_uint8* op_ptr = op_col->get_nth<t_uint8>(idx);
                t_op op = static_cast<t_op>(*op_ptr);

                if (op == OP_DELETE)
                    continue;

                t_uindex stableidx = stableidx_vec[idx];

                switch (fcolumn->get_dtype())
                {
                    case DTYPE_NONE:
                    {
                    }
                    break;
                    case DTYPE_INT64:
                    {
                        scolumn->set_nth<t_int64>(
                            stableidx,
                            *(fcolumn->get_nth<t_int64>(idx)));
                    }
                    break;
                    case DTYPE_INT32:
                    {
                        scolumn->set_nth<t_int32>(
                            stableidx,
                            *(fcolumn->get_nth<t_int32>(idx)));
                    }
                    break;
                    case DTYPE_INT16:
                    {
                        scolumn->set_nth<t_int16>(
                            stableidx,
                            *(fcolumn->get_nth<t_int16>(idx)));
                    }
                    break;
                    case DTYPE_INT8:
                    {
                        scolumn->set_nth<t_int8>(
                            stableidx,
                            *(fcolumn->get_nth<t_int8>(idx)));
                    }
                    break;
                    case DTYPE_UINT64:
                    {
                        scolumn->set_nth<t_uint64>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint64>(idx)));
                    }
                    break;
                    case DTYPE_UINT32:
                    {
                        scolumn->set_nth<t_uint32>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint32>(idx)));
                    }
                    break;
                    case DTYPE_UINT16:
                    {
                        scolumn->set_nth<t_uint16>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint16>(idx)));
                    }
                    break;
                    case DTYPE_UINT8:
                    {
                        scolumn->set_nth<t_uint8>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint8>(idx)));
                    }
                    break;
                    case DTYPE_FLOAT64:
                    {
                        scolumn->set_nth<t_float64>(
                            stableidx,
                            *(fcolumn->get_nth<t_float64>(idx)));
                    }
                    break;
                    case DTYPE_FLOAT32:
                    {
                        scolumn->set_nth<t_float32>(
                            stableidx,
                            *(fcolumn->get_nth<t_float32>(idx)));
                    }
                    break;
                    case DTYPE_BOOL:
                    {
                        scolumn->set_nth<t_uint8>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint8>(idx)));
                    }
                    break;
                    case DTYPE_TIME:
                    {
                        scolumn->set_nth<t_int64>(
                            stableidx,
                            *(fcolumn->get_nth<t_int64>(idx)));
                    }
                    break;
                    case DTYPE_DATE:
                    {
                        scolumn->set_nth<t_uint32>(
                            stableidx,
                            *(fcolumn->get_nth<t_uint32>(idx)));
                    }
                    break;
                    case DTYPE_STR:
                    {
                        const char* s =
                            fcolumn->get_nth<const char>(idx);
                        scolumn->set_nth<const char*>(stableidx, s);
                    }
                    break;
                    default:
                    {
                        PSP_COMPLAIN_AND_ABORT("Unexpected type");
                    }
                }
            }
        }
#ifdef PSP_PARALLEL_FOR
        );
#endif
}

void
t_gstate::pprint() const
{
    t_uidxvec indices(m_mapping.size());
    t_uindex idx = 0;
    for (t_mapping::const_iterator iter = m_mapping.begin();
         iter != m_mapping.end();
         ++iter)
    {
        indices[idx] = iter->second;
        ++idx;
    }
    m_table->pprint(indices);
}

t_mask
t_gstate::get_cpp_mask() const
{
    t_uindex sz = m_table->size();
    t_mask msk(sz);
    for (t_mapping::const_iterator iter = m_mapping.begin();
         iter != m_mapping.end();
         ++iter)
    {
        msk.set(iter->second, true);
    }
    return msk;
}

t_table_sptr
t_gstate::get_table()
{
    return m_table;
}

t_table_csptr
t_gstate::get_table() const
{
    return m_table;
}

void
t_gstate::read_column(const t_str& colname,
                      const t_tscalvec& pkeys,
                      t_tscalvec& out_data) const
{
    t_index num = pkeys.size();
    t_col_csptr col = m_table->get_const_column(colname);
    const t_column* col_ = col.get();
    t_tscalvec rval(num);

    for (t_index idx = 0; idx < num; ++idx)
    {
        t_mapping::const_iterator iter = m_mapping.find(pkeys[idx]);
        if (iter != m_mapping.end())
        {
            rval[idx].set(col_->get_scalar(iter->second));
        }
    }

    std::swap(rval, out_data);
}

void
t_gstate::read_column(const t_str& colname,
                      const t_tscalvec& pkeys,
                      t_f64vec& out_data) const
{
    t_index num = pkeys.size();
    t_col_csptr col = m_table->get_const_column(colname);
    const t_column* col_ = col.get();
    t_f64vec rval(num);

    for (t_index idx = 0; idx < num; ++idx)
    {
        t_mapping::const_iterator iter = m_mapping.find(pkeys[idx]);
        if (iter != m_mapping.end())
        {
            rval[idx] = col_->get_scalar(iter->second).to_double();
        }
    }

    std::swap(rval, out_data);
}

t_tscalar
t_gstate::get(t_tscalar pkey, const t_str& colname) const
{
    t_mapping::const_iterator iter = m_mapping.find(pkey);
    if (iter != m_mapping.end())
    {
        t_col_csptr col = m_table->get_const_column(colname);
        return col->get_scalar(iter->second);
    }

    return t_tscalar();
}

t_tscalvec
t_gstate::get_row(t_tscalar pkey) const
{
    auto columns = m_table->get_const_columns();
    t_tscalvec rval(columns.size());

    t_mapping::const_iterator iter = m_mapping.find(pkey);
    PSP_VERBOSE_ASSERT(iter != m_mapping.end(), "Reached end");

    t_uindex ridx = iter->second;
    t_uindex idx = 0;

    for (auto c : columns)
    {
        rval[idx].set(c->get_scalar(ridx));
        ++idx;
    }
    return rval;
}

t_bool
t_gstate::is_unique(const t_tscalvec& pkeys,
                    const t_str& colname,
                    t_tscalar& value) const
{
    t_col_csptr col = m_table->get_const_column(colname);
    const t_column* col_ = col.get();
    value = mknone();

    for (const auto& pkey : pkeys)
    {
        t_mapping::const_iterator iter = m_mapping.find(pkey);
        if (iter != m_mapping.end())
        {
            auto tmp = col_->get_scalar(iter->second);
            if (!value.is_none() && value != tmp)
                return false;
            value = tmp;
        }
    }

    return true;
}

t_bool
t_gstate::apply(
    const t_tscalvec& pkeys,
    const t_str& colname,
    t_tscalar& value,
    std::function<t_bool(const t_tscalar&, t_tscalar&)> fn) const
{
    t_col_csptr col = m_table->get_const_column(colname);
    const t_column* col_ = col.get();

    value = mknone();

    for (const auto& pkey : pkeys)
    {
        t_mapping::const_iterator iter = m_mapping.find(pkey);
        if (iter != m_mapping.end())
        {
            auto tmp = col_->get_scalar(iter->second);
            t_bool done = fn(tmp, value);
            if (done)
            {
                value = tmp;
                return done;
            }
        }
    }

    return false;
}

const t_schema&
t_gstate::get_schema() const
{
    return m_tblschema;
}

t_dtype
t_gstate::get_pkey_dtype() const
{
    if (m_mapping.empty())
        return DTYPE_STR;
    auto iter = m_mapping.begin();
    return iter->first.get_dtype();
}

t_table_sptr
t_gstate::get_pkeyed_table(const t_schema& schema) const
{
    return t_table_sptr(_get_pkeyed_table(schema));
}

t_table_sptr
t_gstate::get_pkeyed_table() const
{
    if (m_mapping.size() == m_table->size())
        return m_table;
    return t_table_sptr(_get_pkeyed_table(m_pkeyed_schema));
}

t_table*
t_gstate::_get_pkeyed_table() const
{
    return _get_pkeyed_table(m_pkeyed_schema);
}

t_table*
t_gstate::_get_pkeyed_table(const t_tscalvec& pkeys) const
{
    return _get_pkeyed_table(m_pkeyed_schema, pkeys);
}

t_table*
t_gstate::_get_pkeyed_table(const t_schema& schema,
                            const t_tscalvec& pkeys) const
{
    t_mask mask(size());

    for (const auto& pkey : pkeys)
    {
        auto lk = lookup(pkey);
        if (lk.m_exists)
        {
            mask.set(lk.m_idx, true);
        }
    }

    return _get_pkeyed_table(schema, mask);
}

t_table*
t_gstate::_get_pkeyed_table(const t_schema& schema) const
{
    auto mask = get_cpp_mask();
    return _get_pkeyed_table(schema, mask);
}

t_table*
t_gstate::_get_pkeyed_table(const t_schema& schema,
                            const t_mask& mask) const
{
    static bool const enable_pkeyed_table_mask_fix = true;
    t_uindex o_ncols = schema.m_columns.size();
    auto sz = enable_pkeyed_table_mask_fix ? mask.count() : mask.size();
    auto rval = new t_table(schema, sz);
    rval->init();
    rval->set_size(sz);

    const auto& sch_cols = schema.m_columns;

    const t_table* tbl = m_table.get();

#ifdef PSP_PARALLEL_FOR
    PSP_PFOR(0,
             int(o_ncols),
             1,
             [&sch_cols, rval, tbl, &mask](int colidx)
#else
    for (t_uindex colidx = 0; colidx < o_ncols; ++colidx)
#endif
             {
                 const t_str& c = sch_cols[colidx];
                 if (c != "psp_op" && c != "psp_pkey")
                 {
                     rval->set_column(
                         c, tbl->get_const_column(c)->clone(mask));
                 }
             }

#ifdef PSP_PARALLEL_FOR
             );
#endif
    auto pkey_col = rval->get_column("psp_pkey").get();
    auto op_col = rval->get_column("psp_op").get();

    op_col->raw_fill<t_uint8>(OP_INSERT);
    op_col->valid_raw_fill(true);
    pkey_col->valid_raw_fill(true);

    std::vector<std::pair<t_tscalar, t_uindex>> order(enable_pkeyed_table_mask_fix ? sz : m_mapping.size());
    if( enable_pkeyed_table_mask_fix )
    {
        std::vector<t_uindex> mapping;
        mapping.resize(mask.size());
        {
            t_uindex mapped = 0;
            for( t_uindex idx = 0; idx < mask.size(); ++idx )
            {
                mapping[idx] = mapped;
                if( mask.get(idx) )
                    ++mapped;
            }
        }

        t_uindex oidx = 0;
        for (const auto& kv : m_mapping)
        {
            if( mask.get( kv.second ) )
            {
                order[oidx] = std::make_pair( kv.first, mapping[kv.second] );
                ++oidx;
            }
        }
    }
    else // enable_pkeyed_table_mask_fix
    {
        t_uindex oidx = 0;
        for (const auto& kv : m_mapping)
        {
            order[oidx] = kv;
            ++oidx;
        }
    }

    std::sort(order.begin(),
              order.end(),
              [](const std::pair<t_tscalar, t_uindex>& a,
                 const std::pair<t_tscalar, t_uindex>& b) {
                  return a.second < b.second;
              });

    if (get_pkey_dtype() == DTYPE_STR)
    {
        static const t_tscalar empty = get_interned_tscalar("");
        static bool const enable_pkeyed_table_vocab_reserve = true;

        t_uindex offset = has_pkey(empty) ? 0 : 1;

        size_t total_string_size = 0;

        if( enable_pkeyed_table_vocab_reserve )
        {
            total_string_size += offset;
            for (t_uindex idx = 0, loop_end = order.size();
                 idx < loop_end;
                 ++idx)
            {
                total_string_size += strlen(order[idx].first.get_char_ptr()) + 1;
            }
        }

        // if the m_mapping is empty, get_pkey_dtype() may lie about our pkeys being strings
        // don't try to reserve in this case
        if( !order.size() )
            total_string_size = 0;

        pkey_col->set_vocabulary(order, total_string_size);
        auto base = pkey_col->get_nth<t_uindex>(0);

        for (t_uindex idx = 0, loop_end = order.size();
             idx < loop_end;
             ++idx)
        {
            base[idx] = idx + offset;
        }
    }
    else
    {
        t_uindex ridx = 0;
        for (const auto& e : order)
        {
            pkey_col->set_scalar(ridx, e.first);
            ++ridx;
        }
    }

    return rval;
}

t_uindex
t_gstate::size() const
{
    return m_table->size();
}

t_tscalvec
t_gstate::get_row_data_pkeys(const t_tscalvec& pkeys) const
{
    t_uindex ncols = m_table->num_columns();
    const t_schema& schema = m_table->get_schema();
    t_tscalvec rval;

    t_colcptrvec columns(ncols);
    for (t_uindex idx = 0, loop_end = schema.size(); idx < loop_end;
         ++idx)
    {
        const t_str& cname = schema.m_columns[idx];
        columns[idx] = m_table->get_const_column(cname).get();
    }

    auto none = mknone();

    for (const auto& pkey : pkeys)
    {
        t_mapping::const_iterator iter = m_mapping.find(pkey);
        if (iter == m_mapping.end())
            continue;

        for (t_uindex cidx = 0; cidx < ncols; ++cidx)
        {
            auto v = columns[cidx]->get_scalar(iter->second);
            if (v.is_valid())
            {
                rval.push_back(v);
            }
            else
            {
                rval.push_back(none);
            }
        }
    }
    return rval;
}

t_bool
t_gstate::has_pkey(t_tscalar pkey) const
{
    return m_mapping.find(pkey) != m_mapping.end();
}

t_tscalvec
t_gstate::has_pkeys(const t_tscalvec& pkeys) const
{
    if (pkeys.empty())
        return t_tscalvec();

    t_tscalvec rval(pkeys.size());
    t_uindex idx = 0;

    for (const auto& p : pkeys)
    {
        t_tscalar tval;
        tval.set(m_mapping.find(p) != m_mapping.end());
        rval[idx].set(tval);
        ++idx;
    }

    return rval;
}

t_tscalvec
t_gstate::get_pkeys() const
{
    t_tscalvec rval(m_mapping.size());
    t_uindex idx = 0;
    for (const auto& kv : m_mapping)
    {
        rval[idx].set(kv.first);
        ++idx;
    }
    return rval;
}

std::pair<t_tscalar, t_tscalar>
get_vec_min_max(const t_tscalvec& vec)
{
    t_tscalar min = mknone();
    t_tscalar max = mknone();

    for (const auto& v : vec)
    {
        if (min.is_none())
        {
            min = v;
        }
        else
        {
            min = std::min(v, min);
        }

        if (max.is_none())
        {
            max = v;
        }
        else
        {
            max = std::max(v, max);
        }
    }

    return std::pair<t_tscalar, t_tscalar>(min, max);
}

t_uindex
t_gstate::mapping_size() const
{
    return m_mapping.size();
}

void
t_gstate::reset()
{
    m_table->clear();
    m_mapping.clear();
    m_free.clear();
}

t_tscalar
t_gstate::get_value(const t_tscalar& pkey, const t_str& colname) const
{
    t_col_csptr col = m_table->get_const_column(colname);
    const t_column* col_ = col.get();
    t_tscalar rval = mknone();

    auto iter = m_mapping.find(pkey);
    if (iter != m_mapping.end())
    {
        rval.set(col_->get_scalar(iter->second));
    }

    return rval;
}

const t_schema&
t_gstate::get_port_schema() const
{
    return m_pkeyed_schema;
}

t_uidxvec
t_gstate::get_pkeys_idx(const t_tscalvec& pkeys) const
{
    t_uidxvec rv;
    rv.reserve(pkeys.size());

    for (const auto& p : pkeys)
    {
        auto lk = lookup(p);
        std::cout << "pkey " << p << " exists " << lk.m_exists
                  << std::endl;
        if (lk.m_exists)
            rv.push_back(lk.m_idx);
    }
    return rv;
}

} // end namespace perspective
