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
#include <perspective/config.h>
#include <perspective/schema.h>
#include <perspective/exports.h>
#include <perspective/min_max.h>
#include <perspective/tracing.h>
#include <perspective/pivot.h>
#include <perspective/shared_ptrs.h>
#include <perspective/step_delta.h>
#include <perspective/slice.h>
#include <perspective/range.h>

namespace perspective
{

template <typename CTX_T>
class t_ctx_common
{
  public:
    t_ctx_common(CTX_T* ctx) : m_ctx(ctx)
    {
    }

    t_slice
    get_data(const t_range& rng, const t_fetchvec& fvec) const
    {
        return m_ctx->unity_get_data(rng, fvec);
    }

  private:
    PSP_NON_COPYABLE(t_ctx_common);
    CTX_T* m_ctx;
};

template <typename DERIVED_T>
class t_ctxbase
{
  public:
    typedef DERIVED_T t_derived;

    t_ctxbase();

    t_ctxbase(const t_schema& schema, const t_config& config);

    void set_name(const t_str& name);
    t_str get_name() const;
    t_int64 get_ptr() const;
    void set_state(t_gstate_sptr state);
    const t_config& get_config() const;
    t_config& get_config();
    t_pivotvec get_pivots() const;

    t_bool get_feature_state(t_ctx_feature feature) const;

    // Backwards compatibility only
    bool get_alerts_enabled() const;
    bool get_deltas_enabled() const;
    bool get_minmax_enabled() const;

    t_bool failed() const;

    t_ctx_common<t_ctxbase>
    common()
    {
        return t_ctx_common<t_ctxbase>(this);
    }

    void unity_populate_slice_row(t_slice& s,
                                  const t_fetchvec& fvec,
                                  t_uindex idx) const;
    void unity_populate_slice_column(t_slice& s,
                                     const t_fetchvec& fvec,
                                     t_uindex idx) const;

    t_slice unity_get_data(const t_range& rng,
                           const t_fetchvec& fvec) const;

    t_tscalvec get_data() const;

  protected:
    t_schema m_schema;
    t_config m_config;
    t_bool m_rows_changed;
    t_bool m_columns_changed;
    t_str m_name;
    t_gstate_sptr m_state;
    t_bool m_init;
    std::vector<t_bool> m_features;
    t_minmaxvec m_minmax;
};

template <typename DERIVED_T>
t_ctxbase<DERIVED_T>::t_ctxbase()
    : m_rows_changed(true), m_columns_changed(true), m_init(false)
{
    m_features = std::vector<t_bool>(CTX_FEAT_LAST_FEATURE);
    m_features[CTX_FEAT_ENABLED] = true;
}

template <typename DERIVED_T>
t_ctxbase<DERIVED_T>::t_ctxbase(const t_schema& schema,
                                const t_config& config)

    : m_schema(schema), m_config(config), m_rows_changed(true),
      m_columns_changed(true), m_init(false)
{
    m_features = std::vector<t_bool>(CTX_FEAT_LAST_FEATURE);
    m_features[CTX_FEAT_ENABLED] = true;
}

template <typename DERIVED_T>
void
t_ctxbase<DERIVED_T>::set_name(const t_str& name)
{
    m_name = name;
}

template <typename DERIVED_T>
t_str
t_ctxbase<DERIVED_T>::get_name() const
{
    return m_name;
}

template <typename DERIVED_T>
t_int64
t_ctxbase<DERIVED_T>::get_ptr() const
{
    return reinterpret_cast<t_int64>(this);
}

template <typename DERIVED_T>
void
t_ctxbase<DERIVED_T>::set_state(t_gstate_sptr state)
{
    m_state = state;
}

template <typename DERIVED_T>
t_config&
t_ctxbase<DERIVED_T>::get_config()
{
    return m_config;
}

template <typename DERIVED_T>
const t_config&
t_ctxbase<DERIVED_T>::get_config() const
{
    return m_config;
}

template <typename DERIVED_T>
t_pivotvec
t_ctxbase<DERIVED_T>::get_pivots() const
{
    return m_config.get_pivots();
}

template <typename DERIVED_T>
t_bool
t_ctxbase<DERIVED_T>::get_alerts_enabled() const
{
    return m_features[CTX_FEAT_ALERT];
}

template <typename DERIVED_T>
t_bool
t_ctxbase<DERIVED_T>::get_deltas_enabled() const
{
    return m_features[CTX_FEAT_DELTA];
}

template <typename DERIVED_T>
t_bool
t_ctxbase<DERIVED_T>::get_minmax_enabled() const
{
    return m_features[CTX_FEAT_MINMAX];
}

template <typename DERIVED_T>
t_bool
t_ctxbase<DERIVED_T>::failed() const
{
    return false;
}

template <typename DERIVED_T>
t_bool
t_ctxbase<DERIVED_T>::get_feature_state(t_ctx_feature feature) const
{
    return m_features[feature];
}

template <typename DERIVED_T>
void
t_ctxbase<DERIVED_T>::unity_populate_slice_column(
    t_slice& s, const t_fetchvec& fvec, t_uindex idx) const
{
    for (auto f : fvec)
    {
        switch (f)
        {
            case FETCH_COLUMN_DEPTH:
            {
                s.column_depth().push_back(
                    reinterpret_cast<const DERIVED_T*>(this)
                        ->unity_get_column_depth(idx));
            }
            break;
            case FETCH_COLUMN_PATHS:
            {
                s.column_paths().push_back(
                    reinterpret_cast<const DERIVED_T*>(this)
                        ->unity_get_column_path(idx));
            }
            break;
            case FETCH_COLUMN_INDICES:
            {
                s.column_indices().push_back(idx);
            }
            break;
            case FETCH_IS_COLUMN_EXPANDED:
            {
                s.is_column_expanded().push_back(
                    reinterpret_cast<const DERIVED_T*>(this)
                        ->unity_get_column_expanded(idx));
            }
            break;
            default:
            {
            }
        }
    }
}

template <typename DERIVED_T>
void
t_ctxbase<DERIVED_T>::unity_populate_slice_row(t_slice& s,
                                               const t_fetchvec& fvec,
                                               t_uindex idx) const
{
    auto cptr = reinterpret_cast<const DERIVED_T*>(this);
    for (auto f : fvec)
    {
        switch (f)
        {
            case FETCH_ROW_PATHS:
            {
                s.row_paths().push_back(
                    cptr->unity_get_row_path(idx));
            }
            break;
            case FETCH_ROW_INDICES:
            {
                s.row_indices().push_back(idx);
            }
            break;
            case FETCH_ROW_DATA_SLICE:
            {
                s.row_data().push_back(cptr->unity_get_row_data(idx));
            }
            break;
            case FETCH_IS_ROW_EXPANDED:
            {
                s.is_row_expanded().push_back(
                    cptr->unity_get_row_expanded(idx));
            }
            break;
            case FETCH_ROW_DEPTH:
            {
                s.row_depth().push_back(
                    cptr->unity_get_row_depth(idx));
            }
            break;
            default:
            {
            }
        }
    }
}

template <typename DERIVED_T>
t_slice
t_ctxbase<DERIVED_T>::unity_get_data(const t_range& rng,
                                     const t_fetchvec& fvec) const
{
    auto cptr = reinterpret_cast<const DERIVED_T*>(this);
    t_uindex row_count = cptr->get_row_count();
    t_uindex bridx, eridx;

    switch (rng.get_mode())
    {
        case RANGE_ROW:
        {
            bridx = rng.bridx();
            eridx = rng.eridx();
        }
        break;
        case RANGE_ALL:
        {
            bridx = 0;
            eridx = row_count;
        }
        break;
        default:
        {
            std::cout << "Unexpected mode encountered => "
                      << rng.get_mode() << std::endl;
            return t_slice();
        }
    }

    if (bridx > eridx || bridx >= row_count || eridx > row_count)
        return t_slice();

    t_slice rv;

    for (t_uindex idx = bridx; idx < eridx; ++idx)
    {
        cptr->unity_populate_slice_row(rv, fvec, idx);
    }

    t_uindex bcidx = 0;
    t_uindex ecidx = cptr->unity_get_column_count();

    for (t_uindex idx = bcidx; idx < ecidx; ++idx)
    {
        cptr->unity_populate_slice_column(rv, fvec, idx);
    }

    return rv;
}

template <typename DERIVED_T>
t_tscalvec
t_ctxbase<DERIVED_T>::get_data() const
{
    auto cptr = reinterpret_cast<const DERIVED_T*>(this);
    return cptr->get_data(
        0, cptr->get_row_count(), 0, cptr->get_column_count());
}

} // end namespace perspective
