/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#pragma once
#include <perspective/portable.h>
SUPPRESS_WARNINGS_VC(4503)

#include <perspective/first.h>
#include <perspective/base.h>
#include <perspective/exports.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <perspective/sort_specification.h>
#include <perspective/sparse_tree_node.h>
#include <perspective/pivot.h>
#include <perspective/aggspec.h>
#include <perspective/step_delta.h>
#include <perspective/min_max.h>
#include <perspective/mask.h>
#include <perspective/sym_table.h>
#include <perspective/shared_ptrs.h>
#include <perspective/table.h>
#include <vector>
#include <algorithm>
#include <deque>
#include <sstream>
#include <queue>

namespace perspective
{

class t_gstate;
class t_dtree_ctx;
class t_config;
class t_ctx2;

using boost::multi_index_container;
using namespace boost::multi_index;

typedef std::pair<t_depth, t_ptidx> t_dptipair;
typedef std::vector<t_dptipair> t_dptipairvec;


struct by_idx
{
};

struct by_depth
{
};

struct by_pidx
{
};

struct by_pidx_hash
{
};

struct by_nstrands
{
};

struct by_idx_pkey
{
};

struct by_idx_lfidx
{
};

PERSPECTIVE_EXPORT t_tscalar get_dominant(t_tscalvec& values);

struct t_build_strand_table_common_rval
{
    t_schema m_flattened_schema;
    t_schema m_strand_schema;
    t_schema m_aggschema;
    t_uindex m_npivotlike;
    t_svec m_pivot_like_columns;
    t_uindex m_pivsize;
};

typedef multi_index_container<
    t_stnode,
    indexed_by<
        ordered_unique<tag<by_idx>,
                       BOOST_MULTI_INDEX_MEMBER(
                           t_stnode, t_uindex, m_idx)>,
        hashed_non_unique<tag<by_depth>,
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_uint8, m_depth)>,
        hashed_non_unique<tag<by_nstrands>,
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_uindex, m_nstrands)>,
        ordered_unique<
            tag<by_pidx>,
            composite_key<t_stnode,
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_uindex, m_pidx),
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_tscalar, m_sort_value),
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_tscalar, m_value)>>,
        ordered_unique<
            tag<by_pidx_hash>,
            composite_key<t_stnode,
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_uindex, m_pidx),
                          BOOST_MULTI_INDEX_MEMBER(
                              t_stnode, t_tscalar, m_value)>>>>
    t_treenodes;

typedef multi_index_container<
    t_stpkey,
    indexed_by<ordered_unique<
        tag<by_idx_pkey>,
        composite_key<
            t_stpkey,
            BOOST_MULTI_INDEX_MEMBER(t_stpkey, t_uindex, m_idx),
            BOOST_MULTI_INDEX_MEMBER(t_stpkey, t_tscalar, m_pkey)>>>>
    t_idxpkey;

typedef multi_index_container<
    t_stleaves,
    indexed_by<ordered_unique<
        tag<by_idx_lfidx>,
        composite_key<t_stleaves,
                      BOOST_MULTI_INDEX_MEMBER(
                          t_stleaves, t_uindex, m_idx),
                      BOOST_MULTI_INDEX_MEMBER(
                          t_stleaves, t_uindex, m_lfidx)>>>>
    t_idxleaf;

typedef std::shared_ptr<t_treenodes> t_sptr_treenodes;
typedef std::shared_ptr<t_idxpkey> t_sptr_idxpkey;
typedef std::shared_ptr<t_idxleaf> t_sptr_idxleaf;

typedef t_treenodes::index<by_idx>::type index_by_idx;
typedef t_treenodes::index<by_pidx>::type index_by_pidx;

typedef t_treenodes::index<by_idx>::type::iterator iter_by_idx;
typedef t_treenodes::index<by_pidx>::type::iterator iter_by_pidx;
typedef t_treenodes::index<by_pidx_hash>::type::iterator
    iter_by_pidx_hash;
typedef std::pair<iter_by_pidx, iter_by_pidx> t_by_pidx_ipair;

typedef t_idxpkey::index<by_idx_pkey>::type::iterator
    iter_by_idx_pkey;

typedef std::pair<iter_by_idx_pkey, iter_by_idx_pkey>
    t_by_idx_pkey_ipair;

struct PERSPECTIVE_EXPORT t_agg_update_info
{
    t_colcptrvec m_src;
    t_colptrvec m_dst;
    t_aggspecvec m_aggspecs;

    std::vector< t_uindex > m_dst_topo_sorted;
};

struct t_tree_unify_rec
{
    t_tree_unify_rec(t_uindex sptidx,
                     t_uindex daggidx,
                     t_uindex saggidx,
                     t_uindex nstrands);

    t_uindex m_sptidx;
    t_uindex m_daggidx;
    t_uindex m_saggidx;
    t_uindex m_nstrands;
};

typedef std::vector<t_tree_unify_rec> t_tree_unify_rec_vec;

class PERSPECTIVE_EXPORT t_stree
{
  public:
    typedef const t_stree* t_cptr;
    typedef std::shared_ptr<t_stree> t_sptr;
    typedef std::shared_ptr<const t_stree> t_csptr;
    typedef t_stnode t_tnode;
    typedef std::vector<t_stnode> t_tnodevec;

    typedef std::map<const char*, const char*, t_cmp_charptr>
        t_sidxmap;

    t_stree(const t_pivotvec& pivots,
            const t_aggspecvec& aggspecs,
            const t_schema& schema,
            const t_config& cfg);
    ~t_stree();

    void init();

    t_str repr() const;

    t_tscalar get_value(t_tvidx idx) const;
    t_tscalar get_sortby_value(t_tvidx idx) const;

    void
    build_strand_table_phase_1(t_tscalar pkey,
                               t_op op,
                               t_uindex idx,
                               t_uindex npivots,
                               t_uindex strand_count_idx,
                               t_uindex aggcolsize,
                               t_bool force_current_row,
                               const t_colcptrvec& piv_pcolcontexts,
                               const t_colcptrvec& piv_tcols,
                               const t_colcptrvec& agg_ccols,
                               const t_colcptrvec& agg_dcols,
                               t_colptrvec& piv_scols,
                               t_colptrvec& agg_acols,
                               t_column* agg_scountspar,
                               t_column* spkey,
                               t_uindex& insert_count,
                               t_bool& pivots_neq,
                               const t_svec& pivot_like) const;

    void build_strand_table_phase_2(t_tscalar pkey,
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
                                    const t_svec& pivot_like) const;

    std::pair<t_table_sptr, t_table_sptr>
    build_strand_table(const t_table& flattened,
                       const t_table& delta,
                       const t_table& prev,
                       const t_table& current,
                       const t_table& transitions,
                       const t_aggspecvec& aggspecs,
                       const t_config& config) const;

    std::pair<t_table_sptr, t_table_sptr>
    build_strand_table(const t_table& flattened,
                       const t_aggspecvec& aggspecs,
                       const t_config& config) const;

    void update_shape_from_static(const t_dtree_ctx& ctx);
    void update_aggs_from_static(const t_dtree_ctx& ctx,
                                 const t_gstate& gstate);

    t_uindex size() const;

    t_uindex get_num_children(t_uindex idx) const;
    void get_child_nodes(t_uindex idx, t_tnodevec& nodes) const;
    t_uidxvec zero_strands() const;

    t_uidxset non_zero_leaves(const t_uidxvec& zero_strands) const;

    t_uidxset non_zero_ids(const t_uidxvec& zero_strands) const;

    t_uidxset non_zero_ids(const t_uidxset& ptiset,
                           const t_uidxvec& zero_strands) const;

    t_uindex get_parent_idx(t_uindex idx) const;
    t_uidxvec get_ancestry(t_uindex idx) const;

    t_index get_sibling_idx(t_tvidx p_ptidx,
                            t_index p_nchild,
                            t_uindex c_ptidx) const;
    t_uindex get_aggidx(t_uindex idx) const;

    t_table_csptr get_aggtable() const;

    t_table* _get_aggtable();

    t_tnode get_node(t_uindex idx) const;

    void get_path(t_uindex idx, t_tscalvec& path) const;
    void get_sortby_path(t_uindex idx, t_tscalvec& path) const;

    t_uindex resolve_child(t_uindex root,
                           const t_tscalar& datum) const;

    void drop_zero_strands();

    void add_pkey(t_uindex idx, t_tscalar pkey);
    void remove_pkey(t_uindex idx, t_tscalar pkey);
    void add_leaf(t_uindex nidx, t_uindex lfidx);
    void remove_leaf(t_uindex nidx, t_uindex lfidx);

    t_by_idx_pkey_ipair get_pkeys_for_leaf(t_uindex idx) const;
    t_depth get_depth(t_uindex ptidx) const;
    void get_drd_indices(t_uindex ridx,
                         t_depth rel_depth,
                         t_uidxvec& leaves) const;
    t_uidxvec get_leaves(t_uindex idx) const;
    t_tscalvec get_pkeys(t_uindex idx) const;
    t_uidxvec get_child_idx(t_uindex idx) const;
    t_ptipairvec get_child_idx_depth(t_uindex idx) const;

    void populate_leaf_index(const t_uidxset& leaves);

    t_uindex last_level() const;

    const t_pivotvec& get_pivots() const;
    t_ptidx resolve_path(t_uindex root, const t_tscalvec& path) const;

    // aggregates should be presized to be same size
    // as agg_indices
    void get_aggregates_for_sorting(t_uindex nidx,
                        const t_idxvec& agg_indices,
                        t_tscalvec& aggregates,
                        t_ctx2 *) const;

    t_tscalar get_aggregate(t_ptidx idx, t_index aggnum) const;

    void get_child_indices(t_ptidx idx, t_ptivec& out_data) const;

    void set_alerts_enabled(bool enabled_state);

    void set_deltas_enabled(bool enabled_state);

    void set_minmax_enabled(bool enabled_state);

    void set_feature_state(t_ctx_feature feature, t_bool state);

    template <typename ITER_T>
    t_minmax get_agg_min_max(ITER_T biter,
                             ITER_T eiter,
                             t_uindex aggidx) const;
    t_minmax get_agg_min_max(t_uindex aggidx, t_depth depth) const;
    t_minmaxvec get_min_max() const;

    void clear_deltas();

    const t_sptr_tcdeltas& get_deltas() const;

    void clear();

    t_tscalar first_last_helper(t_uindex nidx,
                                const t_aggspec& spec,
                                const t_gstate& gstate) const;

    t_bool node_exists(t_uindex nidx);

    t_table* get_aggtable();

    void clear_aggregates(const t_uidxvec& indices);

    std::pair<iter_by_idx, bool> insert_node(const t_tnode& node);
    t_bool has_deltas() const;
    void set_has_deltas(t_bool v);

    t_uidxvec get_descendents(t_uindex nidx) const;

  protected:
    void mark_zero_desc();
    t_uindex get_num_aggcols() const;
    typedef std::pair<const t_column*, t_column*> t_srcdst_columns;
    typedef std::vector<t_srcdst_columns> t_srcdst_colvec;

    t_bool pivots_changed(t_value_transition t) const;
    t_uindex genidx();
    t_uindex gen_aggidx();
    t_uidxvec get_children(t_uindex idx) const;
    void update_agg_table(t_uindex nidx,
                          t_agg_update_info& info,
                          t_uindex src_ridx,
                          t_uindex dst_ridx,
                          t_index nstrands,
                          const t_gstate& gstate);

    t_bool is_leaf(t_uindex nidx) const;

    t_build_strand_table_common_rval
    build_strand_table_common(const t_table& flattened,
                              const t_aggspecvec& aggspecs,
                              const t_config& config) const;

    void populate_pkey_idx(const t_dtree_ctx& ctx,
                           const t_dtree& dtree,
                           t_uindex dptidx,
                           t_uindex sptidx,
                           t_uindex ndepth,
                           t_idxpkey& new_idx_pkey);

  private:
    t_pivotvec m_pivots;
    t_bool m_init;
    t_sptr_treenodes m_nodes;
    t_sptr_idxpkey m_idxpkey;
    t_sptr_idxleaf m_idxleaf;
    t_uindex m_curidx;
    t_table_sptr m_aggregates;
    t_aggspecvec m_aggspecs;
    t_schema m_schema;
    t_uidxvec m_agg_freelist;
    t_uindex m_cur_aggidx;
    t_uidxset m_newids;
    t_uidxset m_newleaves;
    t_sidxmap m_smap;
    t_colcptrvec m_aggcols;
    t_uindex m_dotcount;
    t_sptr_tcdeltas m_deltas;
    t_minmaxvec m_minmax;
    t_tree_unify_rec_vec m_tree_unification_records;
    std::vector<t_bool> m_features;
    t_symtable m_symtable;
    t_bool m_has_delta;
    t_str m_grand_agg_str;
};

template <typename ITER_T>
t_minmax
t_stree::get_agg_min_max(ITER_T biter,
                         ITER_T eiter,
                         t_uindex aggidx) const
{
    auto aggcols = m_aggregates->get_const_columns();
    auto col = aggcols[aggidx];
    t_minmax minmax;

    for (auto iter = biter; iter != eiter; ++iter)
    {
        if (iter->m_idx == 0)
            continue;
        t_uindex aggidx = iter->m_aggidx;
        t_tscalar v = col->get_scalar(aggidx);

        if (minmax.m_min.is_none())
        {
            minmax.m_min = v;
        }
        else
        {
            minmax.m_min = std::min(v, minmax.m_min);
        }

        if (minmax.m_max.is_none())
        {
            minmax.m_max = v;
        }
        else
        {
            minmax.m_max = std::max(v, minmax.m_max);
        }
    }
    return minmax;
}

typedef std::shared_ptr<t_stree> t_stree_sptr;
typedef std::shared_ptr<const t_stree> t_stree_csptr;
typedef std::vector<t_stree*> t_streeptr_vec;
typedef std::vector<t_stree_csptr> t_stree_csptr_vec;

} // end namespace perspective
