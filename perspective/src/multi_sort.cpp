/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <boost/math/special_functions/fpclassify.hpp>
#include <perspective/base.h>
#include <perspective/multi_sort.h>
#include <perspective/scalar.h>
#include <vector>

namespace perspective
{

t_mselem::t_mselem() : m_pkey(mknone()), m_order(0), m_deleted(false)
{
}

t_mselem::t_mselem(const t_tscalvec& row)
    : m_row(row), m_pkey(mknone()), m_order(0), m_deleted(false)
{
}

t_mselem::t_mselem(const t_tscalvec& row, t_uindex order)
    : m_row(row), m_pkey(mknone()), m_order(order), m_deleted(false)
{
}

t_mselem::t_mselem(const t_tscalar& pkey, const t_tscalvec& row)
    : m_row(row), m_pkey(pkey), m_order(0), m_deleted(false)
{
}

t_mselem::t_mselem(const t_mselem& other)
{
    m_pkey = other.m_pkey;
    m_row = other.m_row;
    m_deleted = other.m_deleted;
    m_order = other.m_order;
}

t_mselem::t_mselem(t_mselem&& other)
{
    m_pkey = other.m_pkey;
    m_row = std::move(other.m_row);
    m_deleted = other.m_deleted;
    m_order = other.m_order;
}

t_mselem&
t_mselem::operator=(const t_mselem& other)
{
    m_pkey = other.m_pkey;
    m_row = other.m_row;
    m_deleted = other.m_deleted;
    m_order = other.m_order;
    return *this;
}

t_mselem&
t_mselem::operator=(t_mselem&& other)
{
    m_pkey = other.m_pkey;
    m_row = std::move(other.m_row);
    m_deleted = other.m_deleted;
    m_order = other.m_order;
    return *this;
}

t_minmax_idx::t_minmax_idx(t_index mn, t_index mx)
    : m_min(mn), m_max(mx)
{
}

// Given a vector return the indices of the
// minimum and maximum elements in it.
t_minmax_idx
get_minmax_idx(const t_tscalvec& vec, t_sorttype stype)
{
    t_minmax_idx rval(-1, -1);

    if (vec.empty())
        return rval;

    // min, max
    std::pair<t_tscalar, t_tscalar> min_max;
    min_max.first = *(vec.begin());
    min_max.second = *(vec.begin());

    switch (stype)
    {
        case SORTTYPE_DESCENDING:
        case SORTTYPE_ASCENDING:
        {
            for (t_index idx = 0, loop_end = vec.size();
                 idx < loop_end;
                 ++idx)
            {
                const t_tscalar& val = vec[idx];
                if (val <= min_max.first)
                {
                    min_max.first = val;
                    rval.m_min = idx;
                }

                if (val >= min_max.second)
                {
                    min_max.second = val;
                    rval.m_max = idx;
                }
            }
            return rval;
        }
        break;
        case SORTTYPE_ASCENDING_ABS:
        case SORTTYPE_DESCENDING_ABS:
        {
            for (t_index idx = 0, loop_end = vec.size();
                 idx < loop_end;
                 ++idx)
            {
                t_float64 val = std::abs(vec[idx].to_double());
                t_float64 mindbl = std::abs(t_float64(min_max.first));
                t_float64 maxdbl =
                    std::abs(t_float64(min_max.second));

                if (val <= mindbl)
                {
                    min_max.first.set(val);
                    rval.m_min = idx;
                }

                if (val >= maxdbl)
                {
                    min_max.second.set(val);
                    rval.m_max = idx;
                }
            }
            return rval;
        }
        break;
        case SORTTYPE_NONE:
        {
            rval.m_min = 0;
            rval.m_max = 0;
            return rval;
        }
        break;
        default:
        {
            return rval;
        }
    }

    return rval;
}

t_float64
to_double(const t_tscalar& c)
{
    return c.to_double();
}

t_nancmp::t_nancmp() : m_active(false), m_cmpval(CMP_OP_EQ)
{
}

t_nancmp
nan_compare(t_sorttype order, const t_tscalar& a, const t_tscalar& b)
{
    t_nancmp rval;
    t_bool a_fp = a.is_floating_point();
    t_bool b_fp = b.is_floating_point();

    if (!a_fp && !b_fp)
        return rval;

    t_float64 a_dbl = a.to_double();
    t_float64 b_dbl = b.to_double();

    t_bool a_nan = boost::math::isnan(a_dbl);
    t_bool b_nan = boost::math::isnan(b_dbl);

    rval.m_active = a_nan || b_nan;

    if (!rval.m_active)
        return rval;

    if (a_nan && b_nan)
    {
        rval.m_cmpval = CMP_OP_EQ;
        return rval;
    }

    if (a_nan)
    {
        switch (order)
        {
            case SORTTYPE_NONE:
            case SORTTYPE_ASCENDING:
            case SORTTYPE_ASCENDING_ABS:
            {
                rval.m_cmpval = CMP_OP_LT;
            }
            break;
            case SORTTYPE_DESCENDING_ABS:
            case SORTTYPE_DESCENDING:
            {
                rval.m_cmpval = CMP_OP_GT;
            }
            break;
        }
        return rval;
    }

    switch (order)
    {
        case SORTTYPE_NONE:
        case SORTTYPE_ASCENDING:
        case SORTTYPE_ASCENDING_ABS:
        {
            rval.m_cmpval = CMP_OP_GT;
        }
        break;
        case SORTTYPE_DESCENDING_ABS:
        case SORTTYPE_DESCENDING:
        {
            rval.m_cmpval = CMP_OP_LT;
        }
        break;
    }

    return rval;
}

t_multisorter::t_multisorter(const t_sorttvec& order,
                             t_bool handle_nans)
    : m_sort_order(order), m_handle_nans(handle_nans)
{
}

t_multisorter::t_multisorter(t_mselemvec_csptr elems,
                             const t_sorttvec& order,
                             t_bool handle_nans)
    : m_sort_order(order), m_elems(elems), m_handle_nans(handle_nans)
{
}

t_bool
t_multisorter::operator()(const t_mselem& a, const t_mselem& b) const
{
    return cmp_mselem(a, b, m_sort_order, m_handle_nans);
}

t_bool
t_multisorter::operator()(t_index a, t_index b) const
{
    return this->operator()((*m_elems)[a], (*m_elems)[b]);
}

} // end namespace perspective
