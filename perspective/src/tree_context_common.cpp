/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <perspective/base.h>
#include <perspective/filter.h>
#include <perspective/path.h>
#include <perspective/sparse_tree.h>
#include <perspective/table.h>
#include <perspective/traversal.h>
#include <perspective/env_vars.h>
#include <perspective/dense_tree.h>
#include <perspective/dense_tree_context.h>
#include <unordered_set>

namespace perspective
{

void
notify_sparse_tree_common(t_table_sptr strands,
                          t_table_sptr strand_deltas,
                          t_stree_sptr tree,
                          t_trav_sptr traversal,
                          t_bool process_traversal,
                          const t_aggspecvec& aggregates,
                          const t_sspvec& tree_sortby,
                          const t_sortsvec& ctx_sortby,
                          const t_gstate& gstate)
{
    t_filter fltr;
    if (t_env::log_data_nsparse_strands())
    {
        std::cout << "nsparse_strands" << std::endl;
        strands->pprint();
    }

    if (t_env::log_data_nsparse_strand_deltas())
    {
        std::cout << "nsparse_strand_deltas" << std::endl;
        strand_deltas->pprint();
    }

    auto pivots = tree->get_pivots();

    t_dtree dtree(strands, pivots, tree_sortby);
    dtree.init();

    dtree.check_pivot(fltr, pivots.size() + 1);

    if (t_env::log_data_nsparse_dtree())
    {
        std::cout << "nsparse_dtree" << std::endl;
        dtree.pprint(fltr);
    }

    t_dtree_ctx dctx(strands, strand_deltas, dtree, aggregates);

    dctx.init();

    tree->update_shape_from_static(dctx);

    auto zero_strands = tree->zero_strands();

    t_uindex t_osize = process_traversal ? traversal->size() : 0;
    if (process_traversal)
        traversal->drop_tree_indices(zero_strands);
    t_uindex t_nsize = process_traversal ? traversal->size() : 0;
    if (t_osize != t_nsize)
        tree->set_has_deltas(true);

    auto non_zero_ids = tree->non_zero_ids(zero_strands);
    auto non_zero_leaves = tree->non_zero_leaves(zero_strands);

    tree->drop_zero_strands();

    tree->populate_leaf_index(non_zero_leaves);

    tree->update_aggs_from_static(dctx, gstate);

    t_uidxset visited;

    struct t_leaf_path
    {
        t_tscalvec m_path;
        t_uindex m_lfidx;
    };

    std::vector<t_leaf_path> leaf_paths(non_zero_leaves.size());

    t_uindex count = 0;

    for (auto lfidx : non_zero_leaves)
    {
        leaf_paths[count].m_lfidx = lfidx;
        tree->get_sortby_path(lfidx, leaf_paths[count].m_path);
        std::reverse(leaf_paths[count].m_path.begin(),
                     leaf_paths[count].m_path.end());
        ++count;
    }

    std::sort(leaf_paths.begin(),
              leaf_paths.end(),
              [](const t_leaf_path& a, const t_leaf_path& b) {
                  return a.m_path < b.m_path;
              });

    if (!leaf_paths.empty() && traversal.get() && traversal->size() == 1)
    {
        if ( traversal->get_node( 0 ).m_expanded )
        {
            traversal->populate_root_children( tree );
        }
    }
    else
    {
      for (const auto& lpath : leaf_paths)
    {
        t_uindex lfidx = lpath.m_lfidx;
        auto ancestry = tree->get_ancestry(lfidx);

        t_uindex num_tnodes_existed = 0;

        for (auto nidx : ancestry)
        {
            if (non_zero_ids.find(nidx) == non_zero_ids.end() ||
                visited.find(nidx) != visited.end())
            {
                ++num_tnodes_existed;
            }
            else
            {
                break;
            }
        }

        if (process_traversal)
        {
            traversal->add_node(
                 ctx_sortby, ancestry, num_tnodes_existed);
        }

        for (auto nidx : ancestry)
        {
            visited.insert(nidx);
        }
    }
    }
    
}

void
notify_sparse_tree(t_stree_sptr tree,
                   t_trav_sptr traversal,
                   t_bool process_traversal,
                   const t_aggspecvec& aggregates,
                   const t_sspvec& tree_sortby,
                   const t_sortsvec& ctx_sortby,
                   const t_table& flattened,
                   const t_table& delta,
                   const t_table& prev,
                   const t_table& current,
                   const t_table& transitions,
                   const t_table& existed,
                   const t_config& config,
                   const t_gstate& gstate)
{

    auto strand_values = tree->build_strand_table(flattened,
                                                  delta,
                                                  prev,
                                                  current,
                                                  transitions,
                                                  aggregates,
                                                  config);

    auto strands = strand_values.first;
    auto strand_deltas = strand_values.second;
    notify_sparse_tree_common(strands,
                              strand_deltas,
                              tree,
                              traversal,
                              process_traversal,
                              aggregates,
                              tree_sortby,
                              ctx_sortby,
                              gstate);
}

void
notify_sparse_tree(t_stree_sptr tree,
                   t_trav_sptr traversal,
                   t_bool process_traversal,
                   const t_aggspecvec& aggregates,
                   const t_sspvec& tree_sortby,
                   const t_sortsvec& ctx_sortby,
                   const t_table& flattened,
                   const t_config& config,
                   const t_gstate& gstate)
{
    auto strand_values =
        tree->build_strand_table(flattened, aggregates, config);

    auto strands = strand_values.first;
    auto strand_deltas = strand_values.second;
    notify_sparse_tree_common(strands,
                              strand_deltas,
                              tree,
                              traversal,
                              process_traversal,
                              aggregates,
                              tree_sortby,
                              ctx_sortby,
                              gstate);
}

t_pathvec
ctx_get_expansion_state(t_stree_csptr tree, t_trav_csptr traversal)
{
    t_pathvec paths;
    t_ptivec expanded;
    traversal->get_expanded(expanded);

    for (int i = 0, loop_end = expanded.size(); i < loop_end; i++)
    {
        t_tscalvec path;
        tree->get_path(expanded[i], path);
        paths.push_back(t_path(path));
    }
    return paths;
}

t_tscalvec
ctx_get_path(t_stree_csptr tree, t_trav_csptr traversal, t_tvidx idx)
{
    if (idx < 0 || idx >= t_tvidx(traversal->size()))
    {
        t_tscalvec rval;
        return rval;
    }

    auto tree_index = traversal->get_tree_index(idx);
    t_tscalvec rval;
    tree->get_path(tree_index, rval);
    return rval;
}

t_ftnvec
ctx_get_flattened_tree(t_tvidx idx,
                       t_depth stop_depth,
                       t_traversal& trav,
                       const t_config& config,
                       const t_sortsvec& sortby)
{
    t_ptidx ptidx = trav.get_tree_index(idx);
    trav.expand_to_depth(sortby, stop_depth);
    if (!sortby.empty())
    {
        trav.sort_by(config, sortby, *(trav.get_tree()));
    }
    t_tvidx new_tvidx = trav.tree_index_lookup(ptidx, idx);
    return trav.get_flattened_tree(new_tvidx, stop_depth);
}

} // end namespace perspective
