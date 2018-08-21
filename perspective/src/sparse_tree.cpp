/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <perspective/base.h>
#include <perspective/compat.h>
#include <perspective/extract_aggregate.h>
#include <perspective/multi_sort.h>
#include <perspective/sparse_tree.h>
#include <perspective/sym_table.h>
#include <perspective/tracing.h>
#include <perspective/utils.h>
#include <perspective/env_vars.h>
#include <perspective/dense_tree.h>
#include <perspective/dense_tree_context.h>
#include <perspective/gnode_state.h>
#include <perspective/config.h>
#include <perspective/table.h>
#include <perspective/filter_utils.h>
#include <perspective/context_two.h>
#include <unordered_set>

namespace perspective
{

t_tscalar
get_dominant(t_tscalvec& values)
{
    if (values.empty())
        return mknone();

    std::sort(values.begin(), values.end());

    t_tscalar delem = values[0];
    t_index dcount = 1;
    t_index count = 1;

    for (t_index idx = 1, loop_end = values.size(); idx < loop_end;
         ++idx)
    {
        const t_tscalar& prev = values[idx - 1];
        const t_tscalar& curr = values[idx];

        if (curr == prev && curr.is_valid())
        {
            ++count;
        }

        if ((idx + 1) == static_cast<t_index>(values.size()) ||
            curr != prev)
        {
            if (count > dcount)
            {
                delem = prev;
                dcount = count;
            }

            count = 1;
        }
    }

    return delem;
}

t_tree_unify_rec::t_tree_unify_rec(t_uindex sptidx,
                                   t_uindex daggidx,
                                   t_uindex saggidx,
                                   t_uindex nstrands)
    : m_sptidx(sptidx), m_daggidx(daggidx), m_saggidx(saggidx),
      m_nstrands(nstrands)
{
}

t_stree::t_stree(const t_pivotvec& pivots,
                 const t_aggspecvec& aggspecs,
                 const t_schema& schema,
                 const t_config& cfg)
    : m_pivots(pivots), m_init(false), m_curidx(1),
      m_aggspecs(aggspecs), m_schema(schema), m_cur_aggidx(1),
      m_dotcount(0), m_minmax(aggspecs.size()), m_has_delta(false)
{
    auto g_agg_str = cfg.get_grand_agg_str();
    m_grand_agg_str =
        g_agg_str.empty() ? "Grand Aggregate" : g_agg_str;
}

t_stree::~t_stree()
{
    for (t_sidxmap::iterator iter = m_smap.begin();
         iter != m_smap.end();
         ++iter)
    {
        free(const_cast<char*>(iter->first));
    }
}

t_uindex
t_stree::get_num_aggcols() const
{
    return m_aggspecs.size();
}

t_str
t_stree::repr() const
{
    std::stringstream ss;
    ss << "t_stree<" << this << ">";
    return ss.str();
}

void
t_stree::init()
{
    m_nodes = std::make_shared<t_treenodes>();
    m_idxpkey = std::make_shared<t_idxpkey>();
    m_idxleaf = std::make_shared<t_idxleaf>();

    t_tscalar value =
        m_symtable.get_interned_tscalar(m_grand_agg_str.c_str());
    t_tnode node(0, root_pidx(), value, 0, value, 1, 0);
    m_nodes->insert(node);

    t_svec columns;
    t_dtypevec dtypes;

    for (const auto& spec : m_aggspecs)
    {
        auto cinfo = spec.get_output_specs(m_schema);

        for (const auto& ci : cinfo)
        {
            columns.push_back(ci.m_name);
            dtypes.push_back(ci.m_type);
        }
    }

    t_schema schema(columns, dtypes);

    t_uindex capacity = DEFAULT_EMPTY_CAPACITY;
    m_aggregates = std::make_shared<t_table>(schema, capacity);
    m_aggregates->init();
    m_aggregates->set_size(capacity);

    m_aggcols = t_colcptrvec(columns.size());

    for (t_uindex idx = 0, loop_end = columns.size(); idx < loop_end;
         ++idx)
    {
        m_aggcols[idx] =
            m_aggregates->get_const_column(columns[idx]).get();
    }

    m_deltas = std::make_shared<t_tcdeltas>();
    m_features = std::vector<t_bool>(CTX_FEAT_LAST_FEATURE);
    m_init = true;
}

t_tscalar
t_stree::get_value(t_tvidx idx) const
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(idx);
    PSP_VERBOSE_ASSERT(iter != m_nodes->get<by_idx>().end(),
                       "Reached end iterator");
    return iter->m_value;
}

t_tscalar
t_stree::get_sortby_value(t_tvidx idx) const
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(idx);
    PSP_VERBOSE_ASSERT(iter != m_nodes->get<by_idx>().end(),
                       "Reached end iterator");
    return iter->m_sort_value;
}

void
t_stree::build_strand_table_phase_1(t_tscalar pkey,
                                    t_op op,
                                    t_uindex idx,
                                    t_uindex npivots,
                                    t_uindex strand_count_idx,
                                    t_uindex aggcolsize,
                                    t_bool force_current_row,
                                    const t_colcptrvec& piv_ccols,
                                    const t_colcptrvec& piv_tcols,
                                    const t_colcptrvec& agg_ccols,
                                    const t_colcptrvec& agg_dcols,
                                    t_colptrvec& piv_scols,
                                    t_colptrvec& agg_acols,
                                    t_column* agg_scount,
                                    t_column* spkey,
                                    t_uindex& insert_count,
                                    t_bool& pivots_neq,
                                    const t_svec& pivot_like) const
{
    pivots_neq = false;
    t_sset pivmap;
    t_bool all_eq_tt = true;

    for (t_uindex pidx = 0, ploop_end = pivot_like.size();
         pidx < ploop_end;
         ++pidx)
    {
        const t_str& colname = pivot_like.at(pidx);
        if (pivmap.find(colname) != pivmap.end())
        {
            continue;
        }
        pivmap.insert(colname);
        piv_scols[pidx]->push_back(piv_ccols[pidx]->get_scalar(idx));
        const t_uint8* trans_ =
            piv_tcols[pidx]->get_nth<t_uint8>(idx);
        t_value_transition trans =
            static_cast<t_value_transition>(*trans_);
        if (trans != VALUE_TRANSITION_EQ_TT)
            all_eq_tt = false;

        if (pidx < npivots)
        {
            pivots_neq = pivots_neq || pivots_changed(trans);
        }
    }

    for (t_uindex aggidx = 0; aggidx < aggcolsize; ++aggidx)
    {
        if (aggidx != strand_count_idx)
        {
            if (pivots_neq || force_current_row)
            {
                agg_acols[aggidx]->push_back(
                    agg_ccols[aggidx]->get_scalar(idx));
            }
            else
            {
                agg_acols[aggidx]->push_back(
                    agg_dcols[aggidx]->get_scalar(idx));
            }
        }
    }

    t_int8 cval;

    if (op == OP_DELETE)
    {
        cval = -1;
    }
    else
    {
        if (t_env::backout_force_current_row())
        {
            cval = !all_eq_tt || pivots_neq ? 1 : 0;
        }
        else
        {

            cval = npivots == 0 || !all_eq_tt || pivots_neq ||
                           force_current_row
                       ? 1
                       : 0;
        }
    }
    agg_scount->push_back<t_int8>(cval);
    spkey->push_back(pkey);

    ++insert_count;
}

void
t_stree::build_strand_table_phase_2(t_tscalar pkey,
                                    t_uindex idx,
                                    t_uindex npivots,
                                    t_uindex strand_count_idx,
                                    t_uindex aggcolsize,
                                    const t_colcptrvec& piv_pcols,
                                    const t_colcptrvec& agg_pcols,
                                    t_colptrvec& piv_scols,
                                    t_colptrvec& agg_acols,
                                    t_column* agg_scount,
                                    t_column* spkey,
                                    t_uindex& insert_count,
                                    const t_svec& pivot_like) const
{
    t_sset pivmap;
    for (t_uindex pidx = 0, ploop_end = pivot_like.size();
         pidx < ploop_end;
         ++pidx)
    {
        const t_str& colname = pivot_like.at(pidx);
        if (pivmap.find(colname) != pivmap.end())
        {
            continue;
        }
        pivmap.insert(colname);
        piv_scols[pidx]->push_back(piv_pcols[pidx]->get_scalar(idx));
    }

    for (t_uindex aggidx = 0; aggidx < aggcolsize; ++aggidx)
    {
        if (aggidx != strand_count_idx)
        {
            agg_acols[aggidx]->push_back(
                agg_pcols[aggidx]->get_scalar(idx).negate());
        }
    }

    agg_scount->push_back<t_int8>(t_int8(-1));
    spkey->push_back(pkey);
    ++insert_count;
}

t_build_strand_table_common_rval
t_stree::build_strand_table_common(const t_table& flattened,
                                   const t_aggspecvec& aggspecs,
                                   const t_config& config) const
{
    PSP_TRACE_SENTINEL();
    PSP_VERBOSE_ASSERT(m_init, "touching uninited object");

    t_build_strand_table_common_rval rv;

    rv.m_flattened_schema = flattened.get_schema();
    t_sset sschema_colset;

    for (const auto& piv : m_pivots)
    {
        const t_str& colname = piv.colname();
        t_str sortby_colname = config.get_sort_by(colname);
        auto add_col = [&sschema_colset, &rv](const t_str& cname) {
            if (sschema_colset.find(cname) == sschema_colset.end())
            {
                rv.m_pivot_like_columns.push_back(cname);
                rv.m_strand_schema.add_column(
                    cname, rv.m_flattened_schema.get_dtype(cname));
                sschema_colset.insert(cname);
            }
        };

        add_col(colname);
        add_col(sortby_colname);
    }

    rv.m_pivsize = sschema_colset.size();

    t_sset aggcolset;
    for (const auto& aggspec : aggspecs)
    {
        for (const auto& dep : aggspec.get_dependencies())
        {
            if (dep.type() == DEPTYPE_COLUMN)
            {
                const t_str& depname = dep.name();
                aggcolset.insert(depname);

                if (aggspec.is_non_delta() &&
                    sschema_colset.find(depname) ==
                        sschema_colset.end())
                {
                    rv.m_pivot_like_columns.push_back(depname);
                    rv.m_strand_schema.add_column(
                        depname,
                        rv.m_flattened_schema.get_dtype(depname));
                    sschema_colset.insert(depname);
                }
            }
        }
    }

    rv.m_npivotlike = sschema_colset.size();
    rv.m_strand_schema.add_column(
        "psp_pkey",
        flattened.get_const_column("psp_pkey")->get_dtype());

    for (const auto& aggcol : aggcolset)
    {
        rv.m_aggschema.add_column(
            aggcol, rv.m_flattened_schema.get_dtype(aggcol));
    }

    rv.m_aggschema.add_column("psp_strand_count", DTYPE_INT8);
    return rv;
}

// can contain additional rows
// notably pivot changed rows will be added
std::pair<t_table_sptr, t_table_sptr>
t_stree::build_strand_table(const t_table& flattened,
                            const t_table& delta,
                            const t_table& prev,
                            const t_table& current,
                            const t_table& transitions,
                            const t_aggspecvec& aggspecs,
                            const t_config& config) const
{

    PSP_TRACE_SENTINEL();
    PSP_VERBOSE_ASSERT(m_init, "touching uninited object");

    auto rv = build_strand_table_common(flattened, aggspecs, config);

    // strand table
    t_table_sptr strands =
        std::make_shared<t_table>(rv.m_strand_schema);
    strands->init();

    // strand table
    t_table_sptr aggs = std::make_shared<t_table>(rv.m_aggschema);
    aggs->init();

    t_col_csptr pkey_col = flattened.get_const_column("psp_pkey");
    t_col_csptr op_col = flattened.get_const_column("psp_op");

    t_uindex npivotlike = rv.m_npivotlike;
    t_colcptrvec piv_pcols(npivotlike);
    t_colcptrvec piv_ccols(npivotlike);
    t_colcptrvec piv_tcols(npivotlike);
    t_colptrvec piv_scols(npivotlike);

    t_uindex insert_count = 0;

    for (t_uindex pidx = 0; pidx < npivotlike; ++pidx)
    {
        const t_str& piv = rv.m_strand_schema.m_columns[pidx];
        piv_pcols[pidx] = prev.get_const_column(piv).get();
        piv_ccols[pidx] = current.get_const_column(piv).get();
        piv_tcols[pidx] = transitions.get_const_column(piv).get();
        piv_scols[pidx] = strands->get_column(piv).get();
    }

    t_uindex aggcolsize = rv.m_aggschema.m_columns.size();
    t_colcptrvec agg_ccols(aggcolsize);
    t_colcptrvec agg_pcols(aggcolsize);
    t_colcptrvec agg_dcols(aggcolsize);
    t_colptrvec agg_acols(aggcolsize);

    t_uindex strand_count_idx = 0;

    for (t_uindex aggidx = 0; aggidx < aggcolsize; ++aggidx)
    {
        const t_str& aggcol = rv.m_aggschema.m_columns[aggidx];
        if (aggcol == "psp_strand_count")
        {
            agg_dcols[aggidx] = 0;
            agg_ccols[aggidx] = 0;
            agg_pcols[aggidx] = 0;
            strand_count_idx = aggidx;
        }
        else
        {
            agg_dcols[aggidx] = delta.get_const_column(aggcol).get();
            agg_ccols[aggidx] =
                current.get_const_column(aggcol).get();
            agg_pcols[aggidx] = prev.get_const_column(aggcol).get();
        }

        agg_acols[aggidx] = aggs->get_column(aggcol).get();
    }

    t_column* agg_scount = aggs->get_column("psp_strand_count").get();

    t_column* spkey = strands->get_column("psp_pkey").get();

    t_mask msk_prev, msk_curr;

    if (config.has_filters())
    {
        msk_prev = filter_table_for_config(prev, config);
        msk_curr = filter_table_for_config(current, config);
    }

    t_bool has_filters = config.has_filters();

    if (has_filters)
    {
        for (t_uindex idx = 0, loop_end = flattened.size();
             idx < loop_end;
             ++idx)
        {
            t_bool filter_prev = msk_prev.get(idx);
            t_bool filter_curr = msk_curr.get(idx);

            t_tscalar pkey = pkey_col->get_scalar(idx);
            t_uint8 op_ = *(op_col->get_nth<t_uint8>(idx));
            t_op op = static_cast<t_op>(op_);
            t_bool pivots_neq;

            if (!filter_prev && !filter_curr)
            {
                // nothing to do
                continue;
            }
            else if (!filter_prev && filter_curr)
            {
                // apply current row
                build_strand_table_phase_1(pkey,
                                           op,
                                           idx,
                                           rv.m_pivsize,
                                           strand_count_idx,
                                           aggcolsize,
                                           true,
                                           piv_ccols,
                                           piv_tcols,
                                           agg_ccols,
                                           agg_dcols,
                                           piv_scols,
                                           agg_acols,
                                           agg_scount,
                                           spkey,
                                           insert_count,
                                           pivots_neq,
                                           rv.m_pivot_like_columns);
            }
            else if (filter_prev && !filter_curr)
            {
                // reverse prev row
                build_strand_table_phase_2(pkey,
                                           idx,
                                           rv.m_pivsize,
                                           strand_count_idx,
                                           aggcolsize,
                                           piv_pcols,
                                           agg_pcols,
                                           piv_scols,
                                           agg_acols,
                                           agg_scount,
                                           spkey,
                                           insert_count,
                                           rv.m_pivot_like_columns);
            }
            else if (filter_prev && filter_curr)
            {
                // should be handled as normal
                build_strand_table_phase_1(pkey,
                                           op,
                                           idx,
                                           rv.m_pivsize,
                                           strand_count_idx,
                                           aggcolsize,
                                           false,
                                           piv_ccols,
                                           piv_tcols,
                                           agg_ccols,
                                           agg_dcols,
                                           piv_scols,
                                           agg_acols,
                                           agg_scount,
                                           spkey,
                                           insert_count,
                                           pivots_neq,
                                           rv.m_pivot_like_columns);

                if (op == OP_DELETE || !pivots_neq)
                {
                    continue;
                }

                build_strand_table_phase_2(pkey,
                                           idx,
                                           rv.m_pivsize,
                                           strand_count_idx,
                                           aggcolsize,
                                           piv_pcols,
                                           agg_pcols,
                                           piv_scols,
                                           agg_acols,
                                           agg_scount,
                                           spkey,
                                           insert_count,
                                           rv.m_pivot_like_columns);
            }
        }
    }
    else
    {
        for (t_uindex idx = 0, loop_end = flattened.size();
             idx < loop_end;
             ++idx)
        {

            t_tscalar pkey = pkey_col->get_scalar(idx);
            t_uint8 op_ = *(op_col->get_nth<t_uint8>(idx));
            t_op op = static_cast<t_op>(op_);
            t_bool pivots_neq;

            build_strand_table_phase_1(pkey,
                                       op,
                                       idx,
                                       rv.m_pivsize,
                                       strand_count_idx,
                                       aggcolsize,
                                       false,
                                       piv_ccols,
                                       piv_tcols,
                                       agg_ccols,
                                       agg_dcols,
                                       piv_scols,
                                       agg_acols,
                                       agg_scount,
                                       spkey,
                                       insert_count,
                                       pivots_neq,
                                       rv.m_pivot_like_columns);

            if (op == OP_DELETE || !pivots_neq)
            {
                continue;
            }

            build_strand_table_phase_2(pkey,
                                       idx,
                                       rv.m_pivsize,
                                       strand_count_idx,
                                       aggcolsize,
                                       piv_pcols,
                                       agg_pcols,
                                       piv_scols,
                                       agg_acols,
                                       agg_scount,
                                       spkey,
                                       insert_count,
                                       rv.m_pivot_like_columns);
        }
    }

    strands->reserve(insert_count);
    strands->set_size(insert_count);
    aggs->reserve(insert_count);
    aggs->set_size(insert_count);
    agg_scount->valid_raw_fill(true);
    return std::pair<t_table_sptr, t_table_sptr>(strands, aggs);
}

// can contain additional rows
// notably pivot changed rows will be added
std::pair<t_table_sptr, t_table_sptr>
t_stree::build_strand_table(const t_table& flattened,
                            const t_aggspecvec& aggspecs,
                            const t_config& config) const
{
    PSP_TRACE_SENTINEL();
    PSP_VERBOSE_ASSERT(m_init, "touching uninited object");

    auto rv = build_strand_table_common(flattened, aggspecs, config);

    // strand table
    t_table_sptr strands =
        std::make_shared<t_table>(rv.m_strand_schema);
    strands->init();

    // strand table
    t_table_sptr aggs = std::make_shared<t_table>(rv.m_aggschema);
    aggs->init();

    t_col_csptr pkey_col = flattened.get_const_column("psp_pkey");

    t_col_csptr op_col = flattened.get_const_column("psp_op");

    t_uindex npivotlike = rv.m_npivotlike;
    t_colcptrvec piv_fcols(npivotlike);
    t_colptrvec piv_scols(npivotlike);

    t_uindex insert_count = 0;

    for (t_uindex pidx = 0; pidx < npivotlike; ++pidx)
    {
        const t_str& piv = rv.m_strand_schema.m_columns[pidx];
        piv_fcols[pidx] = flattened.get_const_column(piv).get();
        piv_scols[pidx] = strands->get_column(piv).get();
    }

    t_uindex aggcolsize = rv.m_aggschema.m_columns.size();
    t_colcptrvec agg_fcols(aggcolsize);
    t_colptrvec agg_acols(aggcolsize);

    t_uindex strand_count_idx = 0;

    for (t_uindex aggidx = 0; aggidx < aggcolsize; ++aggidx)
    {
        const t_str& aggcol = rv.m_aggschema.m_columns[aggidx];
        if (aggcol == "psp_strand_count")
        {
            agg_fcols[aggidx] = 0;
            strand_count_idx = aggidx;
        }
        else
        {
            agg_fcols[aggidx] =
                flattened.get_const_column(aggcol).get();
        }

        agg_acols[aggidx] = aggs->get_column(aggcol).get();
    }

    t_column* agg_scount = aggs->get_column("psp_strand_count").get();

    t_column* spkey = strands->get_column("psp_pkey").get();

    t_mask msk;

    if (config.has_filters())
    {
        msk = filter_table_for_config(flattened, config);
    }

    t_bool has_filters = config.has_filters();

    for (t_uindex idx = 0, loop_end = flattened.size();
         idx < loop_end;
         ++idx)
    {
        t_bool filter = !has_filters || msk.get(idx);
        t_tscalar pkey = pkey_col->get_scalar(idx);
        t_uint8 op_ = *(op_col->get_nth<t_uint8>(idx));
        t_op op = static_cast<t_op>(op_);

        if (!filter || op == OP_DELETE)
        {
            // nothing to do
            continue;
        }

        for (t_uindex pidx = 0,
                      ploop_end = rv.m_pivot_like_columns.size();
             pidx < ploop_end;
             ++pidx)
        {
            piv_scols[pidx]->push_back(
                piv_fcols[pidx]->get_scalar(idx));
        }

        for (t_uindex aggidx = 0; aggidx < aggcolsize; ++aggidx)
        {
            if (aggidx != strand_count_idx)
            {
                agg_acols[aggidx]->push_back(
                    agg_fcols[aggidx]->get_scalar(idx));
            }
        }

        agg_scount->push_back<t_int8>(1);
        spkey->push_back(pkey);
        ++insert_count;
    }

    strands->reserve(insert_count);
    strands->set_size(insert_count);
    aggs->reserve(insert_count);
    aggs->set_size(insert_count);
    agg_scount->valid_raw_fill(true);
    return std::pair<t_table_sptr, t_table_sptr>(strands, aggs);
}

t_bool
t_stree::pivots_changed(t_value_transition t) const
{

    return t == VALUE_TRANSITION_NEQ_TF ||
           t == VALUE_TRANSITION_NVEQ_FT ||
           t == VALUE_TRANSITION_NEQ_TT;
}

void
t_stree::populate_pkey_idx(const t_dtree_ctx& ctx,
                           const t_dtree& dtree,
                           t_uindex dptidx,
                           t_uindex sptidx,
                           t_uindex ndepth,
                           t_idxpkey& new_idx_pkey)
{
    if (ndepth == dtree.last_level())
    {
        auto pkey_col = ctx.get_pkey_col();
        auto strand_count_col = ctx.get_strand_count_col();
        auto liters = ctx.get_leaf_iterators(dptidx);

        for (auto lfiter = liters.first; lfiter != liters.second;
             ++lfiter)
        {

            auto lfidx = *lfiter;
            auto pkey = m_symtable.get_interned_tscalar(
                pkey_col->get_scalar(lfidx));
            auto strand_count =
                *(strand_count_col->get_nth<t_int8>(lfidx));

            if (strand_count > 0)
            {
                t_stpkey s(sptidx, pkey);
                new_idx_pkey.insert(s);
            }

            if (strand_count < 0)
            {
                remove_pkey(sptidx, pkey);
            }
        }
    }
}

void
t_stree::update_shape_from_static(const t_dtree_ctx& ctx)
{

    m_newids.clear();
    m_newleaves.clear();
    m_tree_unification_records.clear();

    const t_col_csptr scount =
        ctx.get_aggtable().get_const_column("psp_strand_count_sum");

    const t_dtree& dtree = ctx.get_tree();

    // map dptidx to sptidx
    std::map<t_uindex, t_uindex> nmap;
    nmap[0] = 0;

    t_filter filter;

    // update root
    auto root_iter = m_nodes->get<by_idx>().find(0);
    auto root_node = *root_iter;
    t_index root_nstrands =
        *(scount->get_nth<t_index>(0)) + root_node.m_nstrands;
    root_node.set_nstrands(root_nstrands);
    m_nodes->get<by_idx>().replace(root_iter, root_node);

    t_tree_unify_rec unif_rec(0, 0, 0, root_nstrands);
    m_tree_unification_records.push_back(unif_rec);

    t_idxpkey new_idx_pkey;

    for (auto dptidx : dtree.dfs())
    {
        t_uindex sptidx = 0;
        t_depth ndepth = dtree.get_depth(dptidx);

        if (dptidx == 0)
        {
            populate_pkey_idx(
                ctx, dtree, dptidx, sptidx, ndepth, new_idx_pkey);
            continue;
        }

        t_uindex p_dptidx = dtree.get_parent(dptidx);
        t_uindex p_sptidx = nmap[p_dptidx];

        t_tscalar value = m_symtable.get_interned_tscalar(
            dtree.get_value(filter, dptidx));

        t_tscalar sortby_value = m_symtable.get_interned_tscalar(
            dtree.get_sortby_value(filter, dptidx));
		
        t_uindex src_ridx = dptidx;

        auto iter = m_nodes->get<by_pidx_hash>().find(
            boost::make_tuple(p_sptidx, value));

        auto nstrands = *(scount->get_nth<t_int64>(dptidx));

        if (iter == m_nodes->get<by_pidx_hash>().end() &&
            nstrands < 0)
        {
            continue;
        }

        if (iter == m_nodes->get<by_pidx_hash>().end())
        {
            // create node and enqueue
            sptidx = genidx();
            t_uindex aggsize = m_aggregates->size();
            if (sptidx == aggsize)
            {
                t_float64 scale = 1.3;
                t_uindex new_size = scale * aggsize;
                m_aggregates->extend(new_size);
            }

            t_uindex dst_ridx = gen_aggidx();

            t_tnode node(sptidx,
                         p_sptidx,
                         value,
                         ndepth,
                         sortby_value,
                         nstrands,
                         dst_ridx);

            m_newids.insert(sptidx);

            if (ndepth == dtree.last_level())
            {
                m_newleaves.insert(sptidx);
            }

            auto insert_pair = m_nodes->insert(node);
            if (!insert_pair.second)
            {
                auto failed_because = *(insert_pair.first);
                std::cout << "failed because of " << failed_because
                          << std::endl;
            }
            PSP_VERBOSE_ASSERT(insert_pair.second,
                               "Failed to insert node");
            t_tree_unify_rec unif_rec(
                sptidx, src_ridx, dst_ridx, nstrands);
            m_tree_unification_records.push_back(unif_rec);
        }
        else
        {
            sptidx = iter->m_idx;

            // update node
            t_tnode node = *iter;
            node.set_sort_value(sortby_value);

            t_uindex dst_ridx = node.m_aggidx;

            nstrands = node.m_nstrands + nstrands;

            t_tree_unify_rec unif_rec(
                sptidx, src_ridx, dst_ridx, nstrands);
            m_tree_unification_records.push_back(unif_rec);

            sptidx = iter->m_idx;
            node.set_nstrands(nstrands);

            t_bool replaced =
                m_nodes->get<by_pidx_hash>().replace(iter, node);
            PSP_VERBOSE_ASSERT(replaced, "Failed to replace");
        }

        populate_pkey_idx(
            ctx, dtree, dptidx, sptidx, ndepth, new_idx_pkey);
        nmap[dptidx] = sptidx;
    }

    auto biter = new_idx_pkey.get<by_idx_pkey>().begin();
    auto eiter = new_idx_pkey.get<by_idx_pkey>().end();

    for (auto iter = biter; iter != eiter; ++iter)
    {
        t_stpkey s(iter->m_idx, iter->m_pkey);
        m_idxpkey->insert(s);
    }

    mark_zero_desc();
}

void
t_stree::mark_zero_desc()
{
    auto zeros = zero_strands();
    t_uidxset z_desc;

    for (auto z : zeros)
    {
        auto d = get_descendents(z);
        std::copy(
            d.begin(), d.end(), std::inserter(z_desc, z_desc.end()));
    }

    for (auto n : z_desc)
    {
        auto iter = m_nodes->get<by_idx>().find(n);
        auto node = *iter;
        node.set_nstrands(0);
        m_nodes->get<by_idx>().replace(iter, node);
    }
}

void
t_stree::update_aggs_from_static(const t_dtree_ctx& ctx,
                                 const t_gstate& gstate)
{
    const t_table& src_aggtable = ctx.get_aggtable();

    t_agg_update_info agg_update_info;
    t_schema aggschema = m_aggregates->get_schema();

    for (auto colname : aggschema.m_columns)
    {
        agg_update_info.m_src.push_back(
            src_aggtable.get_const_column(colname).get());
        agg_update_info.m_dst.push_back(
            m_aggregates->get_column(colname).get());
        agg_update_info.m_aggspecs.push_back(
            ctx.get_aggspec(colname));
    }

    auto is_col_scaled_aggregate = [&]( int col_idx ) -> bool
    {
        int agg_type = agg_update_info.m_aggspecs[ col_idx ].agg();

        return agg_type == AGGTYPE_SCALED_DIV
            || agg_type == AGGTYPE_SCALED_ADD
            || agg_type == AGGTYPE_SCALED_MUL;
    };

    size_t col_cnt = aggschema.m_columns.size();
    auto &cols_topo_sorted = agg_update_info.m_dst_topo_sorted;
    cols_topo_sorted.clear();
    cols_topo_sorted.reserve(col_cnt);

    static bool const enable_aggregate_reordering = true; 
    static bool const enable_fix_double_calculation = true; 

    std::unordered_set< t_column* > dst_visited;
    auto push_column = [&]( size_t idx )
    {
        if ( enable_fix_double_calculation )
        {
            t_column* dst = agg_update_info.m_dst[ idx ];
            if ( dst_visited.find( dst ) != dst_visited.end() )
            {
                return;
            }
            dst_visited.insert( dst );
        }
        cols_topo_sorted.push_back( idx );
    };

    if ( enable_aggregate_reordering )
    { 
    // Move scaled agg columns to the end
    // This does not handle case where scaled aggregate depends on other scaled aggregate
    // ( not sure if that is possible )
    for (size_t i = 0; i < col_cnt; ++i)
    {
        if (!is_col_scaled_aggregate(i))
        {
            push_column(i);
        }
    }
    for (size_t i = 0; i < col_cnt; ++i)
    {
        if (is_col_scaled_aggregate(i))
        {
            push_column(i);
        }
    }
    }
    else
    {
        // If backed out, use same column order as before ( not topo sorted )
        for ( size_t i = 0; i < col_cnt; ++i )
        {
            push_column( i );
        }
    }

    for (const auto& r : m_tree_unification_records)
    {
        if (!node_exists(r.m_sptidx))
        {
            continue;
        }

        update_agg_table(r.m_sptidx,
                         agg_update_info,
                         r.m_daggidx,
                         r.m_saggidx,
                         r.m_nstrands,
                         gstate);
    }
}

t_uindex
t_stree::genidx()
{
    return m_curidx++;
}

t_uidxvec
t_stree::get_children(t_uindex idx) const
{
    t_by_pidx_ipair iterators =
        m_nodes->get<by_pidx>().equal_range(idx);

    t_uindex nchild =
        std::distance(iterators.first, iterators.second);

    t_uidxvec temp(nchild);

    t_index count = 0;
    for (iter_by_pidx iter = iterators.first;
         iter != iterators.second;
         ++iter)
    {
        temp[count] = iter->m_idx;
        ++count;
    }
    return temp;
}

t_uindex
t_stree::size() const
{
    return m_nodes->size();
}

void
t_stree::get_child_nodes(t_uindex idx, t_tnodevec& nodes) const
{
    t_index num_children = get_num_children(idx);
    t_tnodevec temp(num_children);
    auto iterators = m_nodes->get<by_pidx>().equal_range(idx);
    std::copy(iterators.first, iterators.second, temp.begin());
    std::swap(nodes, temp);
}

t_uindex
t_stree::get_num_children(t_uindex ptidx) const
{
    auto iterators = m_nodes->get<by_pidx>().equal_range(ptidx);
    return std::distance(iterators.first, iterators.second);
}

t_uindex
t_stree::gen_aggidx()
{
    if (!m_agg_freelist.empty())
    {
        t_uindex rval = m_agg_freelist.back();
        m_agg_freelist.pop_back();
        return rval;
    }

    t_uindex cur_cap = m_aggregates->size();
    t_uindex rval = m_cur_aggidx;
    ++m_cur_aggidx;
    if (rval >= cur_cap)
    {
        t_float64 nrows = ceil(.3 * t_float64(rval));
        m_aggregates->extend(static_cast<t_uindex>(nrows));
    }

    return rval;
}

void
t_stree::update_agg_table(t_uindex nidx,
                          t_agg_update_info& info,
                          t_uindex src_ridx,
                          t_uindex dst_ridx,
                          t_index nstrands,
                          const t_gstate& gstate)
{
    static bool const enable_sticky_nan_fix = true;
    for (t_uindex idx : info.m_dst_topo_sorted)
    {
        const t_column* src = info.m_src[idx];
        t_column* dst = info.m_dst[idx];
        const t_aggspec& spec = info.m_aggspecs[idx];
        t_tscalar new_value = mknone();
        t_tscalar old_value = mknone();

        switch (spec.agg())
        {
            case AGGTYPE_PCT_SUM_PARENT:
            case AGGTYPE_PCT_SUM_GRAND_TOTAL:
            case AGGTYPE_SUM:
            {
                t_tscalar src_scalar = src->get_scalar(src_ridx);
                t_tscalar dst_scalar = dst->get_scalar(dst_ridx);
                old_value.set(dst_scalar);
                new_value.set(dst_scalar.add(src_scalar));
                if( enable_sticky_nan_fix && old_value.is_nan() ) // is_nan returns false for non-float types
                {
                    // if we previously had a NaN, add can't make it finite again; recalculate entire sum in case it is now finite
                    auto pkeys = get_pkeys(nidx);
                    t_f64vec values;
                    gstate.read_column(spec.get_dependencies()[0].name(), pkeys, values);
                    new_value.set(std::accumulate(values.begin(), values.end(), t_float64(0)));
                }
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_COUNT:
            {
                if (nidx == 0)
                {
                    new_value.set(nstrands - 1);
                }
                else
                {
                    new_value.set(nstrands);
                }

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_MEAN:
            {
                auto pkeys = get_pkeys(nidx);
                t_f64vec values;

                gstate.read_column(
                    spec.get_dependencies()[0].name(), pkeys, values);

                auto nr = std::accumulate(
                    values.begin(), values.end(), t_float64(0));
                t_float64 dr = values.size();

                t_f64pair* dst_pair =
                    dst->get_nth<t_f64pair>(dst_ridx);

                old_value.set(dst_pair->first / dst_pair->second);

                dst_pair->first = nr;
                dst_pair->second = dr;

                dst->set_valid(dst_ridx, true);

                new_value.set(nr / dr);
            }
            break;
            case AGGTYPE_WEIGHTED_MEAN:
            {
                auto pkeys = get_pkeys(nidx);

                t_f64vec values;
                t_f64vec weights;

                gstate.read_column(
                    spec.get_dependencies()[0].name(), pkeys, values);

                gstate.read_column(spec.get_dependencies()[1].name(),
                                   pkeys,
                                   weights);

                t_float64 init_value = 0.0;

                auto nr = std::inner_product(weights.begin(),
                                             weights.end(),
                                             values.begin(),
                                             init_value);

                auto dr = std::accumulate(
                    weights.begin(), weights.end(), t_float64(0));

                t_f64pair* dst_pair =
                    dst->get_nth<t_f64pair>(dst_ridx);

                old_value.set(dst_pair->first / dst_pair->second);

                dst_pair->first = nr;
                dst_pair->second = dr;

                dst->set_valid(dst_ridx, true);

                new_value.set(nr / dr);
            }
            break;
            case AGGTYPE_UNIQUE:
            {
                auto pkeys = get_pkeys(nidx);
                old_value.set(dst->get_scalar(dst_ridx));

                t_bool is_unique = gstate.is_unique(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    new_value);

                if (new_value.m_type == DTYPE_STR)
                {
                    if (is_unique)
                    {
                        new_value = m_symtable.get_interned_tscalar(
                            new_value);
                    }
                    else
                    {
                        new_value =
                            m_symtable.get_interned_tscalar("-");
                    }
                    dst->set_scalar(dst_ridx, new_value);
                }
                else
                {
                    if (is_unique)
                    {
                        dst->set_scalar(dst_ridx, new_value);
                    }
                    else
                    {
                        dst->set_valid(dst_ridx, false);
                        new_value = old_value;
                    }
                }
            }
            break;
            case AGGTYPE_OR:
            case AGGTYPE_ANY:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);
                gstate.apply(pkeys,
                             spec.get_dependencies()[0].name(),
                             new_value,
                             [](const t_tscalar& row_value,
                                t_tscalar& output) {
                                 if (row_value)
                                 {
                                     output.set(row_value);
                                     return true;
                                 }
                                 return false;
                             });

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_MEDIAN:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        if (values.size() == 0)
                        {
                            return t_tscalar();
                        }
                        else if (values.size() == 1)
                        {
                            return values[0];
                        }
                        else
                        {
                            t_tscalvec::iterator middle =
                                values.begin() + (values.size() / 2);

                            std::nth_element(
                                values.begin(), middle, values.end());

                            return *middle;
                        }
                    }));

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_JOIN:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [this](t_tscalvec& values) {
                        t_tscalset vset;
                        for (const auto& v : values)
                        {
                            vset.insert(v);
                        }

                        std::stringstream ss;
                        for (t_tscalset::const_iterator iter =
                                 vset.begin();
                             iter != vset.end();
                             ++iter)
                        {
                            ss << *iter << ", ";
                        }
                        return m_symtable.get_interned_tscalar(
                            ss.str().c_str());
                    }));

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_SCALED_DIV:
            {
                const t_column* src_1 =
                    info.m_dst[spec.get_agg_one_idx()];
                const t_column* src_2 =
                    info.m_dst[spec.get_agg_two_idx()];

                t_column* dst = info.m_dst[idx];
                old_value.set(dst->get_scalar(dst_ridx));

                t_float64 agg1 =
                    src_1->get_scalar(dst_ridx).to_double();
                t_float64 agg2 =
                    src_2->get_scalar(dst_ridx).to_double();

                t_float64 w1 = spec.get_agg_one_weight();
                t_float64 w2 = spec.get_agg_two_weight();

                t_float64 v = (agg1 * w1) / (agg2 * w2);

                new_value.set(v);
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_SCALED_ADD:
            {

                const t_column* src_1 =
                    info.m_dst[spec.get_agg_one_idx()];
                const t_column* src_2 =
                    info.m_dst[spec.get_agg_two_idx()];

                t_column* dst = info.m_dst[idx];
                old_value.set(dst->get_scalar(dst_ridx));

                t_float64 v =
                    (src_1->get_scalar(dst_ridx).to_double() *
                     spec.get_agg_one_weight()) +
                    (src_2->get_scalar(dst_ridx).to_double() *
                     spec.get_agg_two_weight());

                new_value.set(v);
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_SCALED_MUL:
            {
                const t_column* src_1 =
                    info.m_dst[spec.get_agg_one_idx()];
                const t_column* src_2 =
                    info.m_dst[spec.get_agg_two_idx()];

                t_column* dst = info.m_dst[idx];
                old_value.set(dst->get_scalar(dst_ridx));

                t_float64 v =
                    (src_1->get_scalar(dst_ridx).to_double() *
                     spec.get_agg_one_weight()) *
                    (src_2->get_scalar(dst_ridx).to_double() *
                     spec.get_agg_two_weight());

                new_value.set(v);
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_DOMINANT:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        return get_dominant(values);
                    }));

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_FIRST:
            case AGGTYPE_LAST:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                new_value.set(first_last_helper(nidx, spec, gstate));
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_AND:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        t_tscalar rval;
                        rval.set(true);

                        for (const auto& v : values)
                        {
                            if (!v)
                            {
                                rval.set(false);
                                break;
                            }
                        }
                        return rval;
                    }));
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_LAST_VALUE:
            {
                t_tscalar src_scalar = src->get_scalar(src_ridx);
                t_tscalar dst_scalar = dst->get_scalar(dst_ridx);

                old_value.set(dst_scalar);
                new_value.set(src_scalar);

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_HIGH_WATER_MARK:
            {
                t_tscalar src_scalar = src->get_scalar(src_ridx);
                t_tscalar dst_scalar = dst->get_scalar(dst_ridx);

                old_value.set(dst_scalar);
                new_value.set(src_scalar);

                if (dst_scalar.is_valid())
                {
                    new_value.set(std::max(dst_scalar, src_scalar));
                }

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_LOW_WATER_MARK:
            {
                t_tscalar src_scalar = src->get_scalar(src_ridx);
                t_tscalar dst_scalar = dst->get_scalar(dst_ridx);

                old_value.set(dst_scalar);
                new_value.set(src_scalar);

                if (dst_scalar.is_valid())
                {
                    new_value.set(std::min(dst_scalar, src_scalar));
                }
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_UDF_COMBINER:
            case AGGTYPE_UDF_REDUCER:
            {
                // these will be filled in later
            }
            break;
            case AGGTYPE_SUM_NOT_NULL:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        if (values.empty())
                        {
                            return mknone();
                        }

                        t_tscalar rval;
                        rval.set(t_uint64(0));
                        rval.m_type = values[0].m_type;
                        for (const auto& v : values)
                        {
                            if (rval.is_nan())
                                continue;
                            rval = rval.add(v);
                        }
                        return rval;
                    }));
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_SUM_ABS:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        if (values.empty())
                        {
                            return mknone();
                        }

                        t_tscalar rval;
                        rval.set(t_uint64(0));
                        rval.m_type = values[0].m_type;
                        for (const auto& v : values)
                        {
                            rval = rval.add(v.abs());
                        }
                        return rval;
                    }));
                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_MUL:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);
                new_value.set(gstate.reduce<
                              std::function<t_tscalar(t_tscalvec&)>>(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    [](t_tscalvec& values) {
                        if (values.size() == 0)
                        {
                            return t_tscalar();
                        }
                        else if (values.size() == 1)
                        {
                            return values[0];
                        }
                        else
                        {
                            t_tscalar v = values[0];
                            for (t_uindex vidx = 1,
                                          vloop_end = values.size();
                                 vidx < vloop_end;
                                 ++vidx)
                            {
                                v = v.mul(values[vidx]);
                            }
                            return v;
                        }
                    }));

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_DISTINCT_COUNT:
            {
                old_value.set(dst->get_scalar(dst_ridx));
                auto pkeys = get_pkeys(nidx);

                new_value.set(
                    gstate
                        .reduce<std::function<t_uint32(t_tscalvec&)>>(
                            pkeys,
                            spec.get_dependencies()[0].name(),
                            [this](t_tscalvec& values) {
                                std::unordered_set<t_tscalar> vset;
                                for (const auto& v : values)
                                {
                                    vset.insert(v);
                                }
                                t_uint32 rv = vset.size();
                                return rv;
                            }));

                dst->set_scalar(dst_ridx, new_value);
            }
            break;
            case AGGTYPE_DISTINCT_LEAF:
            {
                auto pkeys = get_pkeys(nidx);
                old_value.set(dst->get_scalar(dst_ridx));
                t_bool skip = false;
                t_bool is_unique = gstate.is_unique(
                    pkeys,
                    spec.get_dependencies()[0].name(),
                    new_value);

                if (is_leaf(nidx) && is_unique)
                {
                    if (new_value.m_type == DTYPE_STR)
                    {
                        new_value = m_symtable.get_interned_tscalar(
                            new_value);
                    }
                }
                else
                {
                    if (new_value.m_type == DTYPE_STR)
                    {
                        new_value =
                            m_symtable.get_interned_tscalar("");
                    }
                    else
                    {
                        dst->set_valid(dst_ridx, false);
                        new_value = old_value;
                        skip = true;
                    }
                }
                if (!skip)
                    dst->set_scalar(dst_ridx, new_value);
            }
            break;
            default:
            {
                PSP_COMPLAIN_AND_ABORT("Not implemented");
            }
        } // end switch

        t_bool val_neq = old_value != new_value;

        m_has_delta = m_has_delta || val_neq;
        t_bool deltas_enabled = m_features.at(CTX_FEAT_DELTA);
        if (deltas_enabled && val_neq)
        {
            m_deltas->insert(
                t_tcdelta(nidx, idx, old_value, new_value));
        }

    } // end for
}

t_uidxvec
t_stree::zero_strands() const
{
    auto iterators = m_nodes->get<by_nstrands>().equal_range(0);
    t_uidxvec rval;

    for (auto& iter = iterators.first; iter != iterators.second;
         ++iter)
    {
        rval.push_back(iter->m_idx);
    }
    return rval;
}

t_uidxset
t_stree::non_zero_leaves(const t_uidxvec& zero_strands) const
{
    return non_zero_ids(m_newleaves, zero_strands);
}

t_uidxset
t_stree::non_zero_ids(const t_uidxvec& zero_strands) const
{
    return non_zero_ids(m_newids, zero_strands);
}

t_uidxset
t_stree::non_zero_ids(const t_uidxset& ptiset,
                      const t_uidxvec& zero_strands) const
{
    t_uidxset zeroset;
    for (auto idx : zero_strands)
    {
        zeroset.insert(idx);
    }

    t_uidxset rval;

    for (const auto& newid : ptiset)
    {
        if (zeroset.find(newid) == zeroset.end())
        {
            rval.insert(newid);
        }
    }

    return rval;
}

t_uindex
t_stree::get_parent_idx(t_uindex ptidx) const
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(ptidx);
    if (iter == m_nodes->get<by_idx>().end())
    {
        std::cout << "Failed in tree => " << repr() << std::endl;
        PSP_VERBOSE_ASSERT(false, "Did not find node");
    }
    return iter->m_pidx;
}

t_uidxvec
t_stree::get_ancestry(t_uindex idx) const
{
    t_uindex rpidx = root_pidx();
    t_uidxvec rval;

    while (idx != rpidx)
    {
        rval.push_back(idx);
        idx = get_parent_idx(idx);
    }

    std::reverse(rval.begin(), rval.end());
    return rval;
}

t_index
t_stree::get_sibling_idx(t_tvidx p_ptidx,
                         t_index p_nchild,
                         t_uindex c_ptidx) const
{
    t_by_pidx_ipair iterators =
        m_nodes->get<by_pidx>().equal_range(p_ptidx);
    iter_by_pidx c_iter = m_nodes->project<by_pidx>(
        m_nodes->get<by_idx>().find(c_ptidx));
    return std::distance(iterators.first, c_iter);
}

t_uindex
t_stree::get_aggidx(t_uindex idx) const
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(idx);
    PSP_VERBOSE_ASSERT(iter != m_nodes->get<by_idx>().end(),
                       "Failed in get_aggidx");
    return iter->m_aggidx;
}

t_table_csptr
t_stree::get_aggtable() const
{
    return m_aggregates;
}

t_table*
t_stree::_get_aggtable()
{
    return m_aggregates.get();
}

t_stree::t_tnode
t_stree::get_node(t_uindex idx) const
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(idx);
    PSP_VERBOSE_ASSERT(iter != m_nodes->get<by_idx>().end(),
                       "Failed in get_node");
    return *iter;
}

void
t_stree::get_path(t_uindex idx, t_tscalvec& rval) const
{
    t_uindex curidx = idx;

    if (curidx == 0)
        return;

    while (1)
    {
        iter_by_idx iter = m_nodes->get<by_idx>().find(curidx);
        rval.push_back(iter->m_value);
        curidx = iter->m_pidx;
        if (curidx == 0)
        {
            break;
        }
    }
    return;
}

t_uindex
t_stree::resolve_child(t_uindex root, const t_tscalar& datum) const
{
    auto iter = m_nodes->get<by_pidx_hash>().find(
        boost::make_tuple(root, datum));

    if (iter == m_nodes->get<by_pidx_hash>().end())
    {
        return INVALID_INDEX;
    }

    return iter->m_idx;
}

void
t_stree::clear_aggregates(const t_uidxvec& indices)
{
    auto cols = m_aggregates->get_columns();
    for (auto c : cols)
    {
        for (auto aggidx : indices)
        {
            c->set_valid(aggidx, false);
        }
    }

    m_agg_freelist.insert(std::end(m_agg_freelist),
                          std::begin(indices),
                          std::end(indices));
}

void
t_stree::drop_zero_strands()
{
    auto iterators = m_nodes->get<by_nstrands>().equal_range(0);

    t_uidxvec leaves;

    auto lst = last_level();

    t_uidxvec node_ids;

    for (auto iter = iterators.first; iter != iterators.second;
         ++iter)
    {
        if (iter->m_depth == lst)
            leaves.push_back(iter->m_idx);
        node_ids.push_back(iter->m_aggidx);
    }

    clear_aggregates(node_ids);

    for (auto nidx : leaves)
    {
        auto ancestry = get_ancestry(nidx);

        for (auto ancidx : ancestry)
        {
            if (ancidx == nidx)
                continue;
            remove_leaf(ancidx, nidx);
        }
    }

    auto iterators2 = m_nodes->get<by_nstrands>().equal_range(0);

    m_nodes->get<by_nstrands>().erase(iterators2.first,
                                      iterators2.second);
}

void
t_stree::add_pkey(t_uindex idx, t_tscalar pkey)
{
    t_stpkey s(idx, pkey);
    m_idxpkey->insert(s);
}

void
t_stree::remove_pkey(t_uindex idx, t_tscalar pkey)
{
    auto iter = m_idxpkey->get<by_idx_pkey>().find(
        boost::make_tuple(idx, pkey));

    if (iter == m_idxpkey->get<by_idx_pkey>().end())
        return;

    m_idxpkey->get<by_idx_pkey>().erase(iter);
}

void
t_stree::add_leaf(t_uindex nidx, t_uindex lfidx)
{
    t_stleaves s(nidx, lfidx);
    m_idxleaf->insert(s);
}

void
t_stree::remove_leaf(t_uindex nidx, t_uindex lfidx)
{
    auto iter = m_idxleaf->get<by_idx_lfidx>().find(
        boost::make_tuple(nidx, lfidx));

    if (iter == m_idxleaf->get<by_idx_lfidx>().end())
        return;

    m_idxleaf->get<by_idx_lfidx>().erase(iter);
}

t_by_idx_pkey_ipair
t_stree::get_pkeys_for_leaf(t_uindex idx) const
{
    return m_idxpkey->get<by_idx_pkey>().equal_range(idx);
}

t_tscalvec
t_stree::get_pkeys(t_uindex idx) const
{
    t_tscalvec rval;
    t_uidxvec leaves = get_leaves(idx);

    for (auto leaf : leaves)
    {
        auto iters = get_pkeys_for_leaf(leaf);
        for (auto iter = iters.first; iter != iters.second; ++iter)
        {
            rval.push_back(iter->m_pkey);
        }
    }
    return rval;
}

t_uidxvec
t_stree::get_leaves(t_uindex idx) const
{
    t_uidxvec rval;

    if (is_leaf(idx))
    {
        rval.push_back(idx);
        return rval;
    }

    auto iters = m_idxleaf->get<by_idx_lfidx>().equal_range(idx);
    for (auto iter = iters.first; iter != iters.second; ++iter)
    {
        rval.push_back(iter->m_lfidx);
    }
    return rval;
}

t_depth
t_stree::get_depth(t_uindex ptidx) const
{
    auto iter = m_nodes->get<by_idx>().find(ptidx);
    return iter->m_depth;
}

void
t_stree::get_drd_indices(t_uindex ridx,
                         t_depth rel_depth,
                         t_uidxvec& leaves) const
{
    t_ptipairvec pending;

    if (rel_depth == 0)
    {
        leaves.push_back(ridx);
        return;
    }

    t_depth rdepth = get_depth(ridx);
    t_depth edepth = rdepth + rel_depth;

    pending.push_back(t_ptipair(ridx, rdepth));

    while (!pending.empty())
    {
        t_ptipair head = pending.back();
        pending.pop_back();

        if (head.second == edepth - 1)
        {
            auto children = get_child_idx(head.first);
            std::copy(children.begin(),
                      children.end(),
                      std::back_inserter(leaves));
        }
        else
        {
            auto children = get_child_idx_depth(head.first);
            std::copy(children.begin(),
                      children.end(),
                      std::back_inserter(pending));
        }
    }
}

t_uidxvec
t_stree::get_child_idx(t_uindex idx) const
{
    t_index num_children = get_num_children(idx);
    t_uidxvec children(num_children);
    auto iterators = m_nodes->get<by_pidx>().equal_range(idx);
    t_index count = 0;
    while (iterators.first != iterators.second)
    {
        children[count] = iterators.first->m_idx;
        ++count;
        ++iterators.first;
    }
    return children;
}

t_ptipairvec
t_stree::get_child_idx_depth(t_uindex idx) const
{
    t_index num_children = get_num_children(idx);
    t_ptipairvec children(num_children);
    auto iterators = m_nodes->get<by_pidx>().equal_range(idx);
    t_index count = 0;
    while (iterators.first != iterators.second)
    {
        children[count] = t_ptipair(iterators.first->m_idx,
                                    iterators.first->m_depth);
        ++count;
        ++iterators.first;
    }
    return children;
}

void
t_stree::populate_leaf_index(const t_uidxset& leaves)
{
    for (auto nidx : leaves)
    {
        t_uidxvec ancestry = get_ancestry(nidx);

        for (auto ancidx : ancestry)
        {
            if (ancidx == nidx)
                continue;

            add_leaf(ancidx, nidx);
        }
    }
}

t_uindex
t_stree::last_level() const
{
    return m_pivots.size();
}

t_bool
t_stree::is_leaf(t_uindex nidx) const
{
    auto iter = m_nodes->get<by_idx>().find(nidx);
    PSP_VERBOSE_ASSERT(iter != m_nodes->get<by_idx>().end(),
                       "Did not find node");
    return iter->m_depth == last_level();
}

t_uidxvec
t_stree::get_descendents(t_uindex nidx) const
{
    t_uidxvec rval;

    t_uidxvec queue;
    queue.push_back(nidx);

    while (!queue.empty())
    {
        auto h = queue.back();
        queue.pop_back();
        auto children = get_children(h);
        queue.insert(std::end(queue),
                     std::begin(children),
                     std::end(children));
        rval.insert(
            std::end(rval), std::begin(children), std::end(children));
    }

    return rval;
}

const t_pivotvec&
t_stree::get_pivots() const
{
    return m_pivots;
}

t_ptidx
t_stree::resolve_path(t_uindex root, const t_tscalvec& path) const
{
    t_ptidx curidx = root;

    if (path.empty())
        return curidx;

    for (t_index i = path.size() - 1; i >= 0; i--)
    {
        iter_by_pidx_hash iter = m_nodes->get<by_pidx_hash>().find(
            boost::make_tuple(curidx, path[i]));
        if (iter == m_nodes->get<by_pidx_hash>().end())
        {
            return INVALID_INDEX;
        }
        curidx = iter->m_idx;
    }

    return curidx;
}

// aggregates should be presized to be same size
// as agg_indices
void
t_stree::get_aggregates_for_sorting(t_uindex nidx,
                        const t_idxvec& agg_indices,
                        t_tscalvec& aggregates,
                        t_ctx2 * ctx2) const
{
    t_uindex aggidx = get_aggidx(nidx);
    for (t_uindex idx = 0, loop_end = agg_indices.size();
         idx < loop_end;
         ++idx)
    {
        auto which_agg = agg_indices[idx];
       if(which_agg < 0)
        {
            aggregates[idx] = get_sortby_value(nidx);
        }
        else if( ctx2 || ( size_t(which_agg) >= m_aggcols.size() ) )
        {
			aggregates[idx].set(t_none());
            if(ctx2)
            {
				if( (ctx2->get_config().get_totals() == TOTALS_BEFORE) && (size_t(which_agg) < m_aggcols.size()) )
				{
					aggregates[idx] = m_aggcols[which_agg]->get_scalar(aggidx);
					continue;
				}

                // two sided pivoted column
                // we don't have enough information here to work out the shape of the data
                // so we have to use ctx2 to resolve

                // fetch the row/column path from ctx2
                t_tscalvec col_path = ctx2->get_column_path_userspace( which_agg+1 );
                if( col_path.empty() )
                {
                    if( ctx2->get_config().get_totals() == TOTALS_AFTER )
                    {
                        aggregates[idx] = m_aggcols[which_agg % m_aggcols.size()]->get_scalar(aggidx);
                    }
                    continue;
                }

                t_tscalvec row_path;
                get_path(nidx, row_path);

                auto target_tree = ctx2->get_trees()[ get_node(nidx).m_depth ];
                t_ptidx target = target_tree->resolve_path( 0, row_path );
                if( target != INVALID_INDEX )
                    target = target_tree->resolve_path( target, col_path );
                if( target != INVALID_INDEX )
                {
                    aggregates[idx] = target_tree->get_aggregate( target, which_agg % m_aggcols.size() );
                }
            }
        }
        else
        {
            aggregates[idx] =
                m_aggcols[which_agg]->get_scalar(aggidx);
        }
    }
}

t_tscalar
t_stree::get_aggregate(t_ptidx idx, t_index aggnum) const
{
    if (aggnum < 0)
    {
        return get_value(idx);
    }

    auto aggtable = get_aggtable();
    auto c = aggtable->get_const_column(aggnum).get();
    auto agg_ridx = get_aggidx(idx);

    t_ptidx pidx = get_parent_idx(idx);

    t_index agg_pridx =
        pidx == INVALID_INDEX ? INVALID_INDEX : get_aggidx(pidx);

    return extract_aggregate(
        m_aggspecs[aggnum], c, agg_ridx, agg_pridx);
}

void
t_stree::get_child_indices(t_ptidx idx, t_ptivec& out_data) const
{
    t_index num_children = get_num_children(idx);
    t_ptivec temp(num_children);
    t_by_pidx_ipair iterators =
        m_nodes->get<by_pidx>().equal_range(idx);
    t_index count = 0;
    for (iter_by_pidx iter = iterators.first;
         iter != iterators.second;
         ++iter)
    {
        temp[count] = iter->m_idx;
        ++count;
    }
    std::swap(out_data, temp);
}

void
t_stree::clear_deltas()
{
    m_deltas->clear();
    m_has_delta = false;
}

void
t_stree::clear()
{
    m_nodes->clear();
    clear_deltas();
}

void
t_stree::set_alerts_enabled(bool enabled_state)
{
    m_features[CTX_FEAT_ALERT] = enabled_state;
}

void
t_stree::set_deltas_enabled(bool enabled_state)
{
    m_features[CTX_FEAT_DELTA] = enabled_state;
}

void
t_stree::set_minmax_enabled(bool enabled_state)
{
    m_features[CTX_FEAT_MINMAX] = enabled_state;
}

void
t_stree::set_feature_state(t_ctx_feature feature, t_bool state)
{
    m_features[feature] = state;
}

t_minmax
t_stree::get_agg_min_max(t_uindex aggidx, t_depth depth) const
{
    auto iterators = m_nodes->get<by_depth>().equal_range(depth);
    return get_agg_min_max(iterators.first, iterators.second, aggidx);
}

t_minmaxvec
t_stree::get_min_max() const
{
    t_uindex naggs = m_aggspecs.size();
    t_minmaxvec rval(naggs);
    for (t_uindex cidx = 0; cidx < naggs; ++cidx)
    {

        auto biter = m_nodes->get<by_idx>().begin();
        auto eiter = m_nodes->get<by_idx>().end();
        rval[cidx] = get_agg_min_max(biter, eiter, cidx);
    }
    return rval;
}

const t_sptr_tcdeltas&
t_stree::get_deltas() const
{
    return m_deltas;
}

t_tscalar
t_stree::first_last_helper(t_uindex nidx,
                           const t_aggspec& spec,
                           const t_gstate& gstate) const
{
    auto pkeys = get_pkeys(nidx);

    if (pkeys.empty())
        return mknone();

    t_tscalvec values;
    t_tscalvec sort_values;

    gstate.read_column(
        spec.get_dependencies()[0].name(), pkeys, values);
    gstate.read_column(
        spec.get_dependencies()[1].name(), pkeys, sort_values);

    auto minmax_idx =
        get_minmax_idx(sort_values, spec.get_sort_type());

    switch (spec.get_sort_type())
    {
        case SORTTYPE_ASCENDING:
        case SORTTYPE_ASCENDING_ABS:
        {
            if (spec.agg() == AGGTYPE_FIRST)
            {
                if (minmax_idx.m_min >= 0)
                {
                    return values[minmax_idx.m_min];
                }
            }
            else
            {
                if (minmax_idx.m_max >= 0)
                {
                    return values[minmax_idx.m_max];
                }
            }
        }
        break;
        case SORTTYPE_DESCENDING:
        case SORTTYPE_DESCENDING_ABS:
        {
            if (spec.agg() == AGGTYPE_FIRST)
            {
                if (minmax_idx.m_max >= 0)
                {
                    return values[minmax_idx.m_max];
                }
            }
            else
            {
                if (minmax_idx.m_min >= 0)
                {
                    return values[minmax_idx.m_min];
                }
            }
        }
        break;
        default:
        {
            // return none
        }
    }

    return mknone();
}

t_bool
t_stree::node_exists(t_uindex idx)
{
    iter_by_idx iter = m_nodes->get<by_idx>().find(idx);
    return iter != m_nodes->get<by_idx>().end();
}

t_table*
t_stree::get_aggtable()
{
    return m_aggregates.get();
}

std::pair<iter_by_idx, t_bool>
t_stree::insert_node(const t_tnode& node)
{
    return m_nodes->insert(node);
}

t_bool
t_stree::has_deltas() const
{
    return m_has_delta;
}

void
t_stree::get_sortby_path(t_uindex idx, t_tscalvec& rval) const
{
    t_uindex curidx = idx;

    if (curidx == 0)
        return;

    while (1)
    {
        iter_by_idx iter = m_nodes->get<by_idx>().find(curidx);
        rval.push_back(iter->m_sort_value);
        curidx = iter->m_pidx;
        if (curidx == 0)
        {
            break;
        }
    }
    return;
}

void
t_stree::set_has_deltas(t_bool v)
{
    m_has_delta = v;
}

} // end namespace perspective
