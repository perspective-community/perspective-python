/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <perspective/traversal.h>
#include <perspective/sparse_tree.h>
#include <perspective/arg_sort.h>
#include <perspective/sort_specification.h>

namespace perspective
{

t_vdnode::t_vdnode() : m_expanded(0), m_depth(INVALID_INDEX)
{
}

t_vdnode::t_vdnode(t_bool expanded, t_depth depth)
    : m_expanded(static_cast<t_index>(expanded)), m_depth(depth)
{
}

t_vdnode::t_vdnode(t_index expanded, t_depth depth)
    : m_expanded(expanded), m_depth(depth)
{
}

t_vdnode::t_vdnode(t_bool expanded, t_bool has_children)
    : m_expanded(expanded), m_has_children(has_children)
{
}

t_traversal::t_traversal(t_stree_csptr tree, t_bool handle_nan_sort)
    : m_tree(tree), m_handle_nan_sort(handle_nan_sort)
{
    t_stnode_vec rchildren;
    tree->get_child_nodes(0, rchildren);
    populate_root_children(rchildren);
}

void
t_traversal::populate_root_children(const t_stnode_vec& rchildren)
{
    m_nodes = std::make_shared<t_tvnvec>(rchildren.size() + 1);

    // Initialize root
    (*m_nodes)[0].m_expanded = true;
    (*m_nodes)[0].m_depth = 0;
    (*m_nodes)[0].m_rel_pidx = INVALID_INDEX;
    (*m_nodes)[0].m_tnid = 0;
    (*m_nodes)[0].m_ndesc = rchildren.size();
    (*m_nodes)[0].m_nchild = rchildren.size();
    
    t_index count = 1;

    for (t_stnode_vec::const_iterator iter = rchildren.begin();
         iter != rchildren.end();
         ++iter)
    {
        t_tvnode& cnode = (*m_nodes)[count];
        cnode.m_expanded = false;
        cnode.m_depth = 1;
        cnode.m_rel_pidx = count;
        cnode.m_tnid = iter->m_idx;
        cnode.m_ndesc = 0;
        cnode.m_nchild = 0;
        count += 1;
    }
}

void
t_traversal::populate_root_children(t_stree_csptr tree)
{
    t_stnode_vec rchildren;
    tree->get_child_nodes(0, rchildren);
    populate_root_children(rchildren);
}

t_index
t_traversal::expand_node(t_tvidx exp_idx)
{
    t_tvnode& exp_tvnode = (*m_nodes)[exp_idx];

    if (exp_tvnode.m_expanded)
    {
        return 0;
    }

    t_stnode_vec tchildren;
    m_tree->get_child_nodes(exp_tvnode.m_tnid, tchildren);
    t_index n_changed = tchildren.size();
    t_tvnvec children = t_tvnvec(n_changed);

    t_index count = 0;
    for (t_stnode_vec::const_iterator iter = tchildren.begin();
         iter != tchildren.end();
         ++iter)
    {
        t_tvnode& tv_node = children[count];
        tv_node.m_expanded = false;
        tv_node.m_depth = exp_tvnode.m_depth + 1;
        tv_node.m_rel_pidx = count + 1;
        tv_node.m_tnid = iter->m_idx;
        tv_node.m_ndesc = 0;
        tv_node.m_nchild = 0;
        count += 1;
    }

    // Update node being expanded
    exp_tvnode.m_expanded = !tchildren.empty();;
    exp_tvnode.m_ndesc += n_changed;
    exp_tvnode.m_nchild = n_changed;

    // insert children of node into the traversal
    m_nodes->insert(m_nodes->begin() + exp_idx + 1,
                    children.begin(),
                    children.end());

    // update ancestors about their new descendents
    update_ancestors(exp_idx, n_changed);
    update_sucessors(exp_idx, n_changed);

    return n_changed;
}

t_index
t_traversal::expand_node(const t_sortsvec& sortby, t_tvidx exp_idx, t_ctx2 * ctx2)
{
    t_tvnode& exp_tvnode = (*m_nodes)[exp_idx];

    if (exp_tvnode.m_expanded)
    {
        return 0;
    }

    t_stnode_vec tchildren;
    m_tree->get_child_nodes(exp_tvnode.m_tnid, tchildren);
    t_index n_changed = tchildren.size();
    t_index count = 0;
    t_idxvec sorted_idx(n_changed);

    t_idxvec sortby_agg_indices(sortby.size());
    t_uindex scount = 0;
    for (const auto& s : sortby)
    {
        sortby_agg_indices[scount] = s.m_agg_index;
        ++scount;
    }

    if (!sortby.empty())
    {
        auto sortelems = std::make_shared<t_mselemvec>(
            static_cast<size_t>(n_changed));
        t_index num_aggs = sortby.size();
        t_tscalvec aggregates(num_aggs);

        t_uindex child_idx = 0;
        for (t_stnode_vec::const_iterator iter = tchildren.begin();
             iter != tchildren.end();
             ++iter)
        {
            m_tree->get_aggregates_for_sorting(
                iter->m_idx, sortby_agg_indices, aggregates, ctx2);
            (*sortelems)[count] = t_mselem(aggregates, child_idx);
            ++count;
            ++child_idx;
        }

        t_sorttvec sort_orders = get_sort_orders(sortby);
        t_multisorter sorter(
            sortelems, sort_orders, m_handle_nan_sort);
        argsort(sorted_idx, sorter);
    }
    else
    {
        for (t_index i = 0, loop_end = sorted_idx.size();
             i != loop_end;
             ++i)
            sorted_idx[i] = i;
    }

    t_tvnvec children = t_tvnvec(n_changed);
    count = 0;
    for (t_index idx = 0, loop_end = sorted_idx.size();
         idx < loop_end;
         ++idx)
    {
        t_tvnode& tv_node = children[count];
        tv_node.m_expanded = false;
        tv_node.m_depth = exp_tvnode.m_depth + 1;
        tv_node.m_rel_pidx = count + 1;
        tv_node.m_tnid = tchildren[sorted_idx[idx]].m_idx;
        tv_node.m_ndesc = 0;
        tv_node.m_nchild = 0;
        count += 1;
    }

    // Update node being expanded
    exp_tvnode.m_expanded = !sorted_idx.empty();
    exp_tvnode.m_ndesc += n_changed;
    exp_tvnode.m_nchild = n_changed;

    // insert children of node into the traversal
    m_nodes->insert(m_nodes->begin() + exp_idx + 1,
                    children.begin(),
                    children.end());

    // update ancestors about their new descendents
    update_ancestors(exp_idx, n_changed);
    update_sucessors(exp_idx, n_changed);

    return n_changed;
}

t_index
t_traversal::collapse_node(t_tvidx idx)
{
    t_tvnode& node = (*m_nodes)[idx];

    if (!node.m_expanded)
    {
        return 0;
    }

    // Calculate span of descendents
    t_index n_changed = node.m_ndesc;

    t_tvidx bidx = idx + 1;
    t_tvidx eidx = bidx + n_changed;

    // remove entries from traversal
    m_nodes->erase(m_nodes->begin() + bidx, m_nodes->begin() + eidx);

    // Update node being collapsed
    node.m_expanded = false;
    node.m_ndesc -= n_changed;
    node.m_nchild = 0;

    // update ancestors about removal of their
    // descendents
    update_ancestors(idx, -n_changed);
    update_sucessors(idx, -n_changed);

    return n_changed;
}

void
t_traversal::add_node(const t_sortsvec& sortby,
                      const t_uidxvec& indices,
                      t_index insert_level_idx,
                      t_ctx2 * ctx2)
{
    t_sortsvec dummy = sortby;
    t_tvivec tv_indices;
    t_tvidx collapsed_ancestor = INVALID_INDEX;

    get_expanded_span(
        indices, tv_indices, collapsed_ancestor, insert_level_idx);

    if (static_cast<t_index>(tv_indices.size()) == insert_level_idx)
    {
        t_tvidx p_tvidx = tv_indices.back();
        const t_tvnode& p_tvnode = (*m_nodes)[p_tvidx];
        t_index p_ptidx = p_tvnode.m_tnid;
        t_index p_nchild = p_tvnode.m_nchild + 1;
        t_ptidx c_ptidx = indices[insert_level_idx];
        t_uindex cidx =
            m_tree->get_sibling_idx(p_ptidx, p_nchild, c_ptidx);
        cidx = std::min(p_tvnode.m_nchild, cidx);
        t_tvidx cur_cidx = p_tvidx + 1;
        for (t_uindex idx = 0; idx < cidx; ++idx)
        {
            cur_cidx += (1 + (*m_nodes)[cur_cidx].m_ndesc);
        }

        (*m_nodes)[p_tvidx].m_nchild += 1;

        t_depth depth = get_depth(p_tvidx) + 1;
        t_tvnode new_node;
        fill_travnode(
            &new_node, false, depth, cur_cidx - p_tvidx, 0, c_ptidx);
        auto insert_iter = m_nodes->begin() + cur_cidx;
        m_nodes->insert(insert_iter, new_node);
        update_ancestors(cur_cidx, 1);
        update_sucessors(cur_cidx, 1);
    }
}

t_rcode
t_traversal::update_ancestors(t_tvidx nidx, t_index n_changed)
{
    if (nidx == 0)
        return 0;

    t_tvidx pidx = nidx - (*m_nodes)[nidx].m_rel_pidx;
    while (pidx > INVALID_INDEX)
    {
        t_tvnode& node = (*m_nodes)[pidx];
        node.m_ndesc += n_changed;
        if (pidx == 0)
        {
            pidx = INVALID_INDEX;
        }
        else
        {
            pidx = pidx - node.m_rel_pidx;
        }
    }
    return 0;
}

t_rcode
t_traversal::update_sucessors(t_tvidx nidx, t_index n_changed)
{
    t_tvnode* c_node = &((*m_nodes)[nidx]);

    while (c_node->m_depth > 0)
    {
        t_tvidx pidx = nidx - c_node->m_rel_pidx;

        const t_tvnode& p_node = (*m_nodes)[pidx];

        t_index p_nchild = p_node.m_nchild;
        t_index coffset = 1;

        for (int i = 0; i < p_nchild; i++)
        {
            t_tvidx curr_cidx = pidx + coffset;
            t_tvnode& child_node = (*m_nodes)[curr_cidx];
            if (curr_cidx > nidx)
            {
                child_node.m_rel_pidx += n_changed;
            }
            if (child_node.m_expanded)
            {
                coffset = coffset + child_node.m_ndesc + 1;
            }
            else
            {
                coffset += 1;
            }
        }

        nidx = pidx;
        c_node = &((*m_nodes)[nidx]);
    }
    return 0;
}

t_ptidx
t_traversal::get_tree_index(t_tvidx idx) const
{
    return (*m_nodes)[idx].m_tnid;
}

t_uindex
t_traversal::size() const
{
    return m_nodes->size();
}

t_depth
t_traversal::get_depth(t_tvidx idx) const
{
    return (*m_nodes)[idx].m_depth;
}

t_tvidx
t_traversal::get_traversal_index(t_ptidx idx)
{
    t_tvidx rval = INVALID_INDEX;

    for (t_index i = 0, loop_end = m_nodes->size(); i < loop_end; ++i)
    {
        if ((*m_nodes)[i].m_tnid == idx)
        {
            rval = i;
            break;
        }
    }
    return rval;
}

t_vdnvec
t_traversal::get_view_nodes(t_tvidx bidx, t_tvidx eidx) const
{
    t_vdnvec vec(eidx - bidx);
    for (t_tvidx i = bidx; i < eidx; i++)
    {
        t_tvidx idx = i - bidx;
        const t_tvnode& tv_node = (*m_nodes)[i];
        vec[idx].m_expanded = tv_node.m_expanded;
        vec[idx].m_depth = tv_node.m_depth;
        t_ptidx tree_idx = get_tree_index(i);
        vec[idx].m_has_children =
            m_tree->get_num_children(tree_idx) > 0;
    }
    return vec;
}

void
t_traversal::get_expanded_span(const t_uidxvec& in_ptidxes,
                               t_tvivec& out_tvidxes,
                               t_tvidx& out_collpsed_ancestor,
                               t_index insert_level_idx)
{
    t_tvidx pidx = 0;
    t_index coffset = 1;

    out_tvidxes.push_back(0);

    for (t_index counter = 1, loop_end = in_ptidxes.size();
         counter < loop_end;
         counter++)
    {
        bool level_node_found = false;
        t_tvidx level_idx = INVALID_INDEX;
        t_index p_nchild = (*m_nodes)[pidx].m_nchild;

        if (counter >= insert_level_idx)
        {
            p_nchild = p_nchild - 1;
        }

        for (t_index cidx = 0; cidx < p_nchild; ++cidx)
        {
            t_tvnode& cnode = (*m_nodes)[pidx + coffset];

            if (static_cast<t_uindex>(cnode.m_tnid) ==
                in_ptidxes[counter])
            {
                level_node_found = true;
                level_idx = pidx + coffset;

                if (cnode.m_expanded)
                {
                    pidx = pidx + coffset;
                    coffset = 1;
                    p_nchild = (*m_nodes)[pidx].m_nchild;
                    out_tvidxes.push_back(pidx);
                    break;
                }
            }
            else
            {
                // TODO: next line
                coffset = coffset + cnode.m_ndesc + 1;
            }
        }

        if (level_node_found && (!((*m_nodes)[level_idx].m_expanded)))
        {
            out_collpsed_ancestor = level_idx;
            break;
        }

        if (!level_node_found)
        {
            break;
        }
    }
}

bool
t_traversal::validate_cells(const t_uidxpvec& cells) const
{
    t_uindex trav_size = size();

    for (t_index idx = 0, loop_end = cells.size(); idx < loop_end;
         ++idx)
    {
        auto r_tvidx = cells[idx].first;
        if (r_tvidx >= trav_size)
            return false;
    }
    return true;
}

t_index
t_traversal::remove_subtree(t_tvidx idx)
{
    t_tvnode& node = (*m_nodes)[idx];

    // Calculate span of descendents
    t_index n_changed = node.m_ndesc + 1;

    t_tvidx bidx = idx;
    t_tvidx eidx = bidx + n_changed;

    update_sucessors(idx, -n_changed);

    // update ancestors about removal of their
    // descendents
    update_ancestors(idx, -n_changed);

    t_tvidx pidx = idx - node.m_rel_pidx;
    (*m_nodes)[pidx].m_nchild -= 1;

    // remove entries from traversal
    m_nodes->erase(m_nodes->begin() + bidx, m_nodes->begin() + eidx);

    return n_changed;
}

void
t_traversal::pprint() const
{
    for (t_index idx = 0, loop_end = m_nodes->size(); idx < loop_end;
         ++idx)
    {
        const t_tvnode& node = (*m_nodes)[idx];
        const t_stnode tnode = m_tree->get_node(node.m_tnid);
        for (t_uindex didx = 0; didx < node.m_depth; didx++)
        {
            std::cout << "\t";
        }
        std::cout << "tvidx: " << idx << " value: " << tnode.m_value
                  << " depth: " << node.m_depth
                  << " m_rel_pidx: " << node.m_rel_pidx
                  << " ndesc: " << node.m_ndesc
                  << " tnid: " << node.m_tnid
                  << " nchild: " << node.m_nchild << std::endl;
    }
}

t_tvnode
t_traversal::get_node(t_tvidx idx) const
{
    return (*m_nodes)[idx];
}

void
t_traversal::get_leaves(t_tvivec& out_data) const
{
    for (t_tvidx curidx = 0, loop_end = m_nodes->size();
         curidx < loop_end;
         ++curidx)
    {
        if (!(*m_nodes)[curidx].m_expanded)
        {
            out_data.push_back(curidx);
        }
    }
}

void
t_traversal::get_child_indices(
    t_tvidx nidx,
    std::vector<std::pair<t_tvidx, t_ptidx>>& out_data) const
{
    const t_tvnode& tvnode = (*m_nodes)[nidx];
    t_index nchild = tvnode.m_nchild;
    t_index coffset = 1;

    for (int i = 0; i < nchild; i++)
    {
        t_tvidx curr_cidx = nidx + coffset;
        const t_tvnode& child_node = (*m_nodes)[curr_cidx];
        out_data.push_back(std::pair<t_tvidx, t_ptidx>(
            curr_cidx, child_node.m_tnid));
        coffset = coffset + child_node.m_ndesc + 1;
    }
}

void
t_traversal::print_stats()
{
    std::cout << "Traversal size => " << m_nodes->size() << std::endl;
}

t_index
t_traversal::get_num_tree_leaves(t_tvidx idx) const
{
    const t_tvnode& node = (*m_nodes)[idx];

    t_index rval = 0;

    for (t_tvidx curidx = idx + 1, loop_end = idx + node.m_ndesc + 1;
         curidx < loop_end;
         ++curidx)
    {
        if (!(*m_nodes)[curidx].m_expanded)
        {
            ++rval;
        }
    }

    return rval;
}

void
t_traversal::post_order(t_tvidx nidx, t_tvivec& out_vec)
{
    std::vector<std::pair<t_tvidx, t_ptidx>> children;
    get_child_indices(nidx, children);

    for (t_index idx = 0, loop_end = children.size(); idx < loop_end;
         ++idx)
    {
        post_order(children[idx].first, out_vec);
    }

    out_vec.push_back(nidx);
}

// Traversal
void
t_traversal::expand_to_depth(const t_sortsvec& sortby, t_depth depth, t_ctx2 * ctx2)
{
    t_ptivec pending;
    depth = depth + 1;
    pending.push_back(0);
    while (pending.size() > 0)
    {
        t_tvidx curidx = pending.back();
        pending.pop_back();
        expand_node(sortby, curidx, ctx2);
        std::vector<std::pair<t_tvidx, t_ptidx>> children;
        get_child_indices(curidx, children);
        for (t_index idx = 0, loop_end = children.size();
             idx < loop_end;
             ++idx)
        {
            const std::pair<t_tvidx, t_ptidx>& child = children[idx];
            if ((*m_nodes)[child.first].m_depth < depth)
            {
                pending.push_back(child.first);
            }
        }
    }
}

void
t_traversal::collapse_to_depth(t_depth depth)
{
    t_index curidx = 1;
    bool loop = true;
    if (static_cast<t_uindex>(curidx) >= m_nodes->size())
        return;
    while (loop)
    {
        const t_tvnode& tv_node = (*m_nodes)[curidx];
        if (tv_node.m_depth > depth)
        {
            collapse_node(curidx);
        }

        curidx += 1;
        loop = curidx < static_cast<t_index>(m_nodes->size());
    }
}

t_ftnvec
t_traversal::get_flattened_tree(t_tvidx idx, t_depth stop_depth) const
{
    std::queue<t_tvidx> queue;
    queue.push(idx);
    t_ftnvec rvec;
    t_index rvec_idx = 1;
    while (!queue.empty())
    {
        t_index hidx = queue.front();
        queue.pop();
        const t_tvnode& c_node = (*m_nodes)[hidx];
        t_depth curdepth = c_node.m_depth;
        t_ftreenode rnode;
        rnode.m_idx = c_node.m_tnid;
        if (curdepth < stop_depth)
        {
            t_index nchild = m_tree->get_num_children(rnode.m_idx);
            rnode.m_fcidx = rvec_idx;
            rnode.m_nchild = nchild;
            rnode.m_depth = c_node.m_depth;
            t_tvidx curr_cidx = hidx + 1;
            t_tvivec children(nchild);
            for (int cidx = 0; cidx < nchild; cidx++)
            {
                const t_tvnode& child_node = (*m_nodes)[curr_cidx];
                children[cidx] = curr_cidx;
                if (child_node.m_expanded)
                {
                    curr_cidx = curr_cidx + child_node.m_ndesc + 1;
                }
                else
                {
                    curr_cidx += 1;
                }
                rvec_idx++;
            }
            for (t_index cidx = 0, loop_end = children.size();
                 cidx < loop_end;
                 ++cidx)
            {
                queue.push(children[cidx]);
            }
        }
        else
        {
            rnode.m_fcidx = 0;
            rnode.m_nchild = 0;
            rnode.m_depth = INVALID_INDEX;
        }
        rvec.push_back(rnode);
    }
    return rvec;
}

t_tvidx
t_traversal::tree_index_lookup(t_ptidx idx, t_tvidx bidx) const
{
    t_tvidx tvidx = INVALID_INDEX;
    for (t_index i = bidx, loop_end = m_nodes->size(); i < loop_end;
         ++i)
    {
        if ((*m_nodes)[i].m_tnid == idx)
        {
            tvidx = i;
            break;
        }
    }
    return tvidx;
}

void
t_traversal::get_node_ancestors(t_tvidx nidx,
                                t_tvivec& ancestors) const
{
    if (nidx == 0)
        return;

    t_tvidx pidx = nidx - (*m_nodes)[nidx].m_rel_pidx;
    while (pidx > INVALID_INDEX)
    {
        ancestors.push_back(pidx);
        const t_tvnode& node = (*m_nodes)[pidx];
        if (pidx == 0)
        {
            pidx = INVALID_INDEX;
        }
        else
        {
            pidx = pidx - node.m_rel_pidx;
        }
    }
}

void
t_traversal::get_expanded(t_ptivec& expanded_tidx) const
{
    // Ancestors of expanded nodes
    std::set<t_tvidx> ancestors;
    t_tvivec expanded;

    if (m_nodes->size() == 0)
        return;

    for (t_index i = m_nodes->size() - 1; i > -1; i--)
    {
        const t_tvnode& node = (*m_nodes)[i];

        if (node.m_expanded && ancestors.find(i) == ancestors.end())
        {
            expanded.push_back(i);
            t_tvivec node_ancestors;
            get_node_ancestors(i, node_ancestors);
            ancestors.insert(node_ancestors.begin(),
                             node_ancestors.end());
        }
    }

    t_ptivec rval(expanded.size());

    for (t_index i = 0, loop_end = rval.size(); i < loop_end; i++)
    {
        const t_tvnode& node = (*m_nodes)[expanded[i]];
        rval[i] = node.m_tnid;
    }

    std::swap(rval, expanded_tidx);
}

void
t_traversal::drop_tree_indices(const t_uidxvec& indices)
{
    for (auto idx : indices)
    {
        t_tvidx tvidx = tree_index_lookup(idx, 0);
        if (tvidx == INVALID_INDEX)
        {
            continue;
        }

        remove_subtree(tvidx);
    }
}

t_bool
t_traversal::is_valid_idx(t_tvidx idx) const
{
    return idx > 0 && idx < t_tvidx(size());
}

const t_stree*
t_traversal::get_tree() const
{
    return m_tree.get();
}

t_bool
t_traversal::get_node_expanded(t_tvidx idx) const
{
    if (idx < 0 || static_cast<t_uindex>(idx) > m_nodes->size())
        return false;
    return m_nodes->at(idx).m_expanded;
}
} // end namespace perspective
