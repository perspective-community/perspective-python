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
#include <perspective/raw_types.h>
#include <perspective/comparators.h>
#include <perspective/mask.h>
#include <perspective/scalar.h>
#include <perspective/exports.h>
#include <boost/scoped_ptr.hpp>
#include <functional>
#include <set>

namespace perspective
{

// Filter operators
template <typename DATA_T, template <typename> class OP_T>
struct t_operator_base
{
    inline bool
    operator()(DATA_T value, DATA_T threshold_value)
    {
        OP_T<DATA_T> cmp;
        return cmp(value, threshold_value);
    }

    inline bool
    operator()(const char* value, const char* threshold_value)
    {
        t_const_char_comparator<OP_T> cmp;
        return cmp(value, threshold_value);
    }
};

template <typename DATA_T>
using t_operator_lt = t_operator_base<DATA_T, std::less>;

template <typename DATA_T>
using t_operator_lteq = t_operator_base<DATA_T, std::less_equal>;

template <typename DATA_T>
using t_operator_gt = t_operator_base<DATA_T, std::greater>;

template <typename DATA_T>
using t_operator_gteq = t_operator_base<DATA_T, std::greater_equal>;

template <typename DATA_T>
using t_operator_ne = t_operator_base<DATA_T, std::not_equal_to>;

template <typename DATA_T>
using t_operator_eq = t_operator_base<DATA_T, std::equal_to>;

template <typename DATA_T, int DTYPE_T>
struct t_operator_in
{
    inline bool
    operator()(const DATA_T& value,
               const std::set<DATA_T>& threshold_values)
    {
        return threshold_values.find(value) != threshold_values.end();
    }
};

template <typename DATA_T, int DTYPE_T>
struct t_operator_begins_with
{
    inline bool
    operator()(DATA_T value, DATA_T threshold_value)
    {
        return false;
    }
};

template <typename DATA_T, int DTYPE_T>
struct t_operator_ends_with
{
    inline bool
    operator()(DATA_T value, DATA_T threshold_value)
    {
        return false;
    }
};

template <typename DATA_T, int DTYPE_T>
struct t_operator_contains
{
    inline bool
    operator()(DATA_T value, DATA_T threshold_value)
    {
        return false;
    }
};

template <>
struct t_operator_begins_with<const t_char*, DTYPE_STR>
{
    inline bool
    operator()(const t_char* value, const t_char* threshold_value)
    {
        t_uindex t_vlen = strlen(value);
        t_uindex t_tlen = strlen(threshold_value);
        return t_vlen < t_tlen
                   ? false
                   : strncmp(value, threshold_value, t_tlen) == 0;
    }
};

template <>
struct t_operator_ends_with<t_uindex, DTYPE_STR>
{
    inline bool
    operator()(const t_char* value, const t_char* threshold_value)
    {
        t_uindex t_vlen = strlen(value);
        t_uindex t_tlen = strlen(threshold_value);

        return t_vlen < t_tlen ? false
                               : strncmp(value + t_vlen - t_tlen,
                                         threshold_value,
                                         t_tlen) == 0;
    }
};

template <>
struct t_operator_contains<t_uindex, DTYPE_STR>
{
    inline bool
    operator()(const t_char* value, const t_char* threshold_value)
    {
        return strstr(value, threshold_value) != 0;
    }
};

struct PERSPECTIVE_EXPORT t_fterm_recipe
{
    t_fterm_recipe()
    {
    }

    t_str m_colname;
    t_filter_op m_op;
    t_tscalar m_threshold;
    t_tscalvec m_bag;
};

typedef std::vector<t_fterm_recipe> t_fterm_recipevec;

struct PERSPECTIVE_EXPORT t_fterm
{
    t_fterm();

    t_fterm(const t_fterm_recipe& v);

    t_fterm(const t_str& colname,
            t_filter_op op,
            t_tscalar threshold,
            const t_tscalvec& bag);

    t_fterm(const t_str& colname,
            t_filter_op op,
            t_tscalar threshold,
            const t_tscalvec& bag,
            t_bool negated,
            t_bool is_primary);

    inline bool
    operator()(t_tscalar s) const
    {
        t_bool rv;
        switch (m_op)
        {
            case FILTER_OP_IN:
            {
                rv = std::find(m_bag.begin(), m_bag.end(), s) !=
                     m_bag.end();
            }
            break;
            default:
            {
                rv = s.cmp(m_op, m_threshold);
            }
            break;
        }

        return m_negated ? (!rv) : rv;
    }

    t_str get_expr() const;

    void coerce_numeric(t_dtype dtype);
    t_fterm_recipe get_recipe() const;

    t_str m_colname;
    t_filter_op m_op;
    t_tscalar m_threshold;
    t_tscalvec m_bag;
    t_bool m_negated;
    t_bool m_is_primary;
    t_bool m_use_interned;
};

typedef std::vector<t_fterm> t_ftermvec;

class PERSPECTIVE_EXPORT t_filter
{
  public:
    t_filter();
    t_filter(const t_svec& columns);

    t_filter(const t_svec& columns, t_uindex bidx, t_uindex eidx);

    t_filter(const t_svec& columns, t_uindex mask_size);
    t_filter(const t_mask& mask);

    t_uindex num_cols() const;
    bool has_filter() const;
    const t_svec& columns() const;
    t_select_mode mode() const;
    t_uindex bidx() const;
    t_uindex eidx() const;
    t_maskcsptr cmask() const;
    t_masksptr mask() const;
    t_uindex count() const;

  private:
    t_select_mode m_mode;
    t_uindex m_bidx;
    t_uindex m_eidx;
    t_svec m_columns;
    t_masksptr m_mask;
};

} // end namespace perspective
