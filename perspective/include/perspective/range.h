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
#include <perspective/raw_types.h>
#include <perspective/base.h>
#include <perspective/scalar.h>
#include <perspective/exports.h>

namespace perspective
{

class PERSPECTIVE_EXPORT t_range
{
  public:
    t_range(t_uindex bridx, t_uindex eridx);

    t_range(t_uindex bridx,
            t_uindex eridx,
            t_uindex bcidx,
            t_uindex beidx);

    t_range(); // select all

    t_range(const t_tscalvec& brpath, const t_tscalvec& erpath);

    t_range(const t_tscalvec& brpath,
            const t_tscalvec& erpath,
            const t_tscalvec& bcpath,
            const t_tscalvec& ecpath);

    t_range(const t_str& expr_name);

    t_uindex bridx() const;
    t_uindex eridx() const;
    t_uindex bcidx() const;
    t_uindex ecidx() const;
    t_range_mode get_mode() const;

  private:
    t_uindex m_bridx;
    t_uindex m_eridx;
    t_uindex m_bcidx;
    t_uindex m_ecidx;
    t_tscalvec m_brpath;
    t_tscalvec m_erpath;
    t_tscalvec m_bcpath;
    t_tscalvec m_ecpath;
    t_str m_expr_name;
    t_range_mode m_mode;
};

} // end namespace perspective
