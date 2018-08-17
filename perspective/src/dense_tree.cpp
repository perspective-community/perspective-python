/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <perspective/dense_tree.h>
#include <perspective/filter.h>
#include <perspective/node_processor.h>
#include <perspective/comparators.h>
#include <perspective/sort_specification.h>
#include <perspective/table.h>

namespace perspective
{

t_dtree::t_dtree(t_dssptr ds,
                 const t_pivotvec& pivots,
                 const t_sspvec& sortby_colvec)
    : m_dirname(""), m_levels_pivoted(0), m_ds(ds), m_pivots(pivots),
      m_nidx(0), m_backing_store(BACKING_STORE_MEMORY), m_init(false),
      m_sortby_colvec(sortby_colvec)
{
}

t_uindex
t_dtree::size() const
{
    return m_nodes.size();
}

t_ptipair
t_dtree::get_level_markers(t_uindex idx) const
{
    PSP_VERBOSE_ASSERT(idx < m_levels.size(), "Unexpected lvlidx");
    return m_levels[idx];
}

t_dtree::t_dtree(const t_str& dirname,
                 t_dssptr ds,
                 const t_pivotvec& pivots,
                 t_backing_store backing_store,
                 const t_sspvec& sortby_colvec)
    : m_dirname(dirname), m_levels_pivoted(0), m_ds(ds),
      m_pivots(pivots), m_nidx(0), m_backing_store(backing_store),
      m_init(false), m_sortby_colvec(sortby_colvec)
{
}

void
t_dtree::init()
{
    t_lstore_recipe leaf_args(m_dirname,
                              leaves_colname(),
                              DEFAULT_CAPACITY,
                              m_backing_store);
    m_leaves =
        t_column(DTYPE_UINT64, false, leaf_args, DEFAULT_CAPACITY);
    m_leaves.init();

    t_lstore_recipe node_args(m_dirname,
                              nodes_colname(),
                              DEFAULT_CAPACITY,
                              m_backing_store);
    m_nodes = t_column(
        DTYPE_USER_FIXED, false, leaf_args, DEFAULT_CAPACITY);
    m_nodes.init();

    m_values = t_colvec(m_pivots.size() + 1);

    m_has_sortby = std::vector<t_bool>(m_values.size());
    m_has_sortby[0] = false;

    m_sortby_columns.clear();
    for (t_uindex sidx = 0, loop_end = m_sortby_colvec.size();
         sidx < loop_end;
         ++sidx)
    {
        m_sortby_columns[m_sortby_colvec[sidx].first] =
            m_sortby_colvec[sidx].second;
    }

    t_lstore_recipe root_args(m_dirname,
                              values_colname("_root_"),
                              DEFAULT_CAPACITY,
                              m_backing_store);

    m_values[0] =
        t_column(DTYPE_STR, false, leaf_args, DEFAULT_CAPACITY);
    m_values[0].init();

    m_sortby_dpthcol.push_back("");

    for (t_uindex idx = 0, loop_end = m_pivots.size(); idx < loop_end;
         ++idx)
    {
        auto colname = m_pivots[idx].colname();
        t_lstore_recipe leaf_args(m_dirname,
                                  values_colname(colname),
                                  DEFAULT_CAPACITY,
                                  m_backing_store);

        auto siter = m_sortby_columns.find(colname);
        t_bool has_sortby = siter != m_sortby_columns.end() &&
                            siter->second != colname;
        m_has_sortby[idx + 1] = has_sortby;
        t_str sortby_column = has_sortby ? siter->second : colname;
        m_sortby_dpthcol.push_back(sortby_column);
        t_dtype dtype = m_ds->get_dtype(colname);
        m_values[idx + 1] =
            t_column(dtype, false, leaf_args, DEFAULT_CAPACITY);
        m_values[idx + 1].init();
    }

    m_init = true;
}

t_str
t_dtree::repr() const
{
    std::stringstream ss;
    ss << m_ds->name() << "_tree_" << this;
    return ss.str();
}

t_str
t_dtree::leaves_colname() const
{
    return repr() + t_str("_leaves");
}

t_str
t_dtree::nodes_colname() const
{
    return repr() + t_str("_nodes");
}

t_str
t_dtree::values_colname(const t_str& tbl_colname) const
{
    return repr() + t_str("_valuespan_") + tbl_colname;
}

void
t_dtree::check_pivot(const t_filter& filter, t_uindex level)
{
    if (level <= m_levels_pivoted)
        return;

    PSP_VERBOSE_ASSERT(level <= m_pivots.size() + 1,
                       "Erroneous level passed in");

    pivot(filter, level);
}

void
t_dtree::pivot(const t_filter& filter, t_uindex level)
{
    if (level <= m_levels_pivoted)
        return;

    PSP_VERBOSE_ASSERT(level <= m_pivots.size() + 1,
                       "Erroneous level passed in");

    t_uindex ncols = m_pivots.size();

    t_uindex nidx = m_nidx;

    t_uindex nrows;
    const t_mask* mask = 0;

    if (ncols > 0)
    {
        if (filter.has_filter())
        {
            nrows = filter.count();
            mask = filter.cmask().get();
        }
        else
        {
            nrows = m_ds->num_rows();
        }
    }
    else
    {
        nrows = m_pivots.empty() ? m_ds->num_rows() : 1;
    }

    t_uindex nbidx, neidx;

    if (m_levels_pivoted == 0)
    {
        m_leaves.extend<t_uindex>(nrows);
        t_uindex* leaves = m_leaves.get_nth<t_uindex>(0);

        for (t_uindex idx = 0; idx < nrows; idx++)
        {
            leaves[idx] = idx;
        }

        nbidx = 0;
        neidx = 1;
    }
    else
    {
        nbidx = m_levels[m_levels_pivoted].first;
        neidx = m_levels[m_levels_pivoted].second;
    }

    for (t_uindex pidx = m_levels_pivoted; pidx < level; pidx++)
    {
        const t_column* pivcol;

        if (pidx == 0)
        {
            t_tnode* root = m_nodes.extend<t_tnode>();
            fill_dense_tnode(root, nidx, nidx, 1, 0, 0, nrows);
            nidx++;
            m_values[0].push_back(t_str("Grand Aggregate"));
            m_levels.push_back(t_uidxpair(nbidx, neidx));
        }
        else
        {
            const t_pivot& pivot = m_pivots[pidx - 1];
            t_str pivot_colname = pivot.colname();
            pivcol = m_ds->get_const_column(pivot_colname).get();
            t_dtype piv_dtype = pivcol->get_dtype();

            t_uindex next_neidx = 0;

            switch (piv_dtype)
            {
                case DTYPE_STR:
                {
                    next_neidx =
                        t_pivot_processor<t_uindex, DTYPE_STR>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                    t_column* value_col = &(m_values[pidx]);
                    value_col->copy_vocabulary(pivcol);
                }
                break;
                case DTYPE_INT64:
                {
                    next_neidx =
                        t_pivot_processor<t_int64, DTYPE_INT64>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_INT32:
                {
                    next_neidx =
                        t_pivot_processor<t_int32, DTYPE_INT32>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_FLOAT64:
                {
                    next_neidx =
                        t_pivot_processor<t_float64, DTYPE_FLOAT64>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_FLOAT32:
                {
                    next_neidx =
                        t_pivot_processor<t_float32, DTYPE_FLOAT32>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_BOOL:
                {
                    next_neidx =
                        t_pivot_processor<t_uint8, DTYPE_BOOL>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_TIME:
                {
                    next_neidx =
                        t_pivot_processor<t_int64, DTYPE_INT64>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                case DTYPE_DATE:
                {
                    next_neidx =
                        t_pivot_processor<t_uint32, DTYPE_UINT32>()(
                            pivcol,
                            &m_nodes,
                            &(m_values[pidx]),
                            &m_leaves,
                            nbidx,
                            neidx,
                            mask);
                }
                break;
                default:
                {
                    PSP_COMPLAIN_AND_ABORT("Not supported yet");
                }
                break;
            }

            nbidx = neidx;
            neidx = next_neidx;
            m_levels.push_back(t_uidxpair(nbidx, neidx));
        }

        m_levels_pivoted = pidx;
    }

    m_nidx = neidx;
}

t_uindex
t_dtree::get_depth(t_ptidx idx) const
{
    return get_span_index(idx).first;
}

void
t_dtree::pprint(const t_filter& filter) const
{
    t_str indent("  ");

    for (auto idx : dfs())
    {
        t_uindex depth = get_depth(idx);

        for (t_uindex didx = 0; didx < depth; ++didx)
        {
            std::cout << indent;
        }

        const t_dense_tnode* node = get_node_ptr(idx);

        std::cout << get_value(filter, idx) << " idx => "
                  << node->m_idx << " pidx => " << node->m_pidx
                  << " fcidx => " << node->m_fcidx << " nchild => "
                  << node->m_nchild << " flidx => " << node->m_flidx
                  << " nleaves => " << node->m_nleaves << std::endl;
    }
}

t_uindex
t_dtree::last_level() const
{
    return m_pivots.size();
}

const t_dtree::t_tnode*
t_dtree::get_node_ptr(t_ptidx nidx) const
{
    return m_nodes.get_nth<t_tnode>(nidx);
}

t_tscalar
t_dtree::_get_value(const t_filter& filter,
                    t_ptidx nidx,
                    bool sort_value) const
{

    t_uidxpair spi = get_span_index(nidx);
    t_index dpth = spi.first;
    t_uindex scalar_idx = spi.second;

    if (sort_value || nidx == 0)
    {
        const t_column& col = m_values[dpth];
        return col.get_scalar(scalar_idx);
    }
    else
    {
        const t_str& colname = m_sortby_dpthcol[dpth];
        const t_column& col =
            *(m_ds->get_const_column(colname).get());
        auto node = get_node_ptr(nidx);
        t_uindex lfidx = *(m_leaves.get_nth<t_uindex>(node->m_flidx));
        return col.get_scalar(lfidx);
    }
}

t_tscalar
t_dtree::get_value(const t_filter& filter, t_ptidx nidx) const
{
    return _get_value(filter, nidx, true);
}

t_tscalar
t_dtree::get_sortby_value(const t_filter& filter, t_ptidx nidx) const
{
    return _get_value(filter, nidx, false);
}

t_uidxpair
t_dtree::get_span_index(t_ptidx idx) const
{
    for (t_uindex i = 0, loop_end = m_levels.size(); i < loop_end;
         i++)
    {
        t_ptidx bidx = m_levels[i].first;
        t_ptidx eidx = m_levels[i].second;

        if ((idx >= bidx) && (idx < eidx))
        {
            return t_uidxpair(i, idx - bidx);
        }
    }

    PSP_COMPLAIN_AND_ABORT("Reached unreachable.");
    return t_uidxpair(0, 0);
}

const t_column*
t_dtree::get_leaf_cptr() const
{
    return &m_leaves;
}

t_bfs_iter<t_dtree>
t_dtree::bfs() const
{
    return t_bfs_iter<t_dtree>(this);
}

t_dfs_iter<t_dtree>
t_dtree::dfs() const
{
    return t_dfs_iter<t_dtree>(this);
}

t_ptidx
t_dtree::get_parent(t_ptidx idx) const
{
    const t_tnode* n = get_node_ptr(idx);
    return n->m_pidx;
}

const t_pivotvec&
t_dtree::get_pivots() const
{
    return m_pivots;
}

} // end namespace perspective
