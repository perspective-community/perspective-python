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
#include <perspective/storage.h>
#include <perspective/exports.h>
#include <perspective/compat.h>
#include <perspective/vocab.h>
#include <functional>
#include <limits>
#include <cmath>
#include <unordered_map>

namespace perspective
{

class PERSPECTIVE_EXPORT t_vocab
{
    typedef std::unordered_map<const char*,
                               t_uindex,
                               t_cchar_umap_hash,
                               t_cchar_umap_cmp>
        t_sidxmap;

  public:
    t_vocab();
    t_vocab(const t_column_recipe& r);
    t_vocab(const t_lstore_recipe& vlendata_recipe,
            const t_lstore_recipe& extents_recipe);
    void rebuild_map();
    void init(t_bool from_recipe);
    t_lstore_sptr get_vlendata();
    t_lstore_sptr get_extents();
    t_uindex get_vlenidx() const;
    t_uindex nbytes() const;
    void verify() const;
    void verify_size() const;
    void fill(const t_lstore& o_vlen,
              const t_lstore& o_extents,
              t_uindex vlenidx);
    t_extent_pair* get_extents_base();
    t_uchar* get_vlen_base();
    void set_vlenidx(t_uindex idx);
    void pprint_vocabulary() const;
    void clone(const t_vocab& v);

    t_uindex get_interned(const t_str& s);
    t_uindex get_interned(const char* s);
    void copy_vocabulary(const t_vocab& other);
    const char*
    unintern_c(t_uindex idx) const;

    t_bool string_exists(const char* c, t_stridx& interned) const;

    void reserve(size_t total_string_size, size_t string_count);

  protected:
    // vlen interface
    t_uindex genidx();

  private:
    // Max string id currently in use
    t_uindex m_vlenidx;
    // varlen

    // Maps a char* to its encoded id
    // Does not own the char* stored in
    // it. value is a t_uindex that maps
    // into m_extents
    t_sidxmap m_map;

    // Stores the vlen as is. for string
    // the trailing zero byte is stored
    // as well
    t_lstore_sptr m_vlendata;

    // Stores begin / end offset pairs for
    // given id. Id mapping is implicit.
    // Pair stored as entry j contains
    // begin and and t_uindex offsets
    // for string with numeric id j.
    // These offsets index into m_vlendata
    t_lstore_sptr m_extents;
};

} // end namespace perspective
