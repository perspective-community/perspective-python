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
#include <perspective/scalar.h>
#include <perspective/histogram.h>
#include <perspective/mask.h>
#include <perspective/compat.h>
#include <perspective/vocab.h>
#include <functional>
#include <limits>
#include <cmath>
#include <unordered_map>

#ifdef PSP_ENABLE_PYTHON
namespace py = boost::python;
namespace np = boost::python::numpy;
#endif

/*
TODO -
1. Implement implicit typepunning based on cardinality.
2. No pointers should be returned from columns. Only
accessors!
3. Add get_nth for strings
*/

namespace perspective
{

enum t_column_storage_mode
{
    COL_STORAGE_MODE_FIX,
    COL_STORAGE_MODE_VAR,
    COL_STORAGE_MODE_ENUM
};

enum t_column_growth_mode
{
    COLUMN_GROWTH_MODE_APPEND,
    COLUMN_GROWTH_MODE_RANDOM_NO_DELETE,
    COLUMN_GROWTH_MODE_RANDOM_WITH_DELETES
};

typedef std::vector<t_column_recipe> t_column_recipe_vec;

struct t_colstr_sort
{
    t_uindex m_curidx;
    t_uindex m_order_idx;
    const char* m_str;
};

typedef std::shared_ptr<t_vocab> t_vocab_sptr;

class t_column;

typedef std::shared_ptr<t_column> t_col_sptr;

#ifdef PSP_COLUMN_VERIFY
#define COLUMN_CHECK_ACCESS(idx)                                     \
    PSP_VERBOSE_ASSERT((idx) <= m_size, "Invalid column access")
#define COLUMN_CHECK_VALUES() verify()
#define COLUMN_CHECK_STRCOL()                                        \
    PSP_VERBOSE_ASSERT(m_isvlen, "Expected to use string column")
#else
#define COLUMN_CHECK_ACCESS(idx)
#define COLUMN_CHECK_VALUES()
#define COLUMN_CHECK_STRCOL()
#endif

class PERSPECTIVE_EXPORT t_column
{
  public:
#ifdef PSP_DBG_MALLOC
    PSP_NEW_DELETE(t_column)
#endif
    t_column();
    t_column(const t_column_recipe& recipe);
    t_column(t_dtype dtype,
             t_bool missing_enabled,
             const t_lstore_recipe& a);
    t_column(t_dtype dtype,
             t_bool missing_enabled,
             const t_lstore_recipe& a,
             t_uindex row_capacity);
    ~t_column();

    void column_copy_helper(const t_column& other);

    t_column(const t_column& c);
    t_column& operator=(const t_column&);

    void init();

    template <typename DATA_T>
    void push_back(DATA_T elem);

    template <typename DATA_T>
    void push_back(DATA_T elem, t_bool valid);

#ifdef PSP_ENABLE_PYTHON
    np::ndarray _as_numpy();
#endif

    t_dtype get_dtype() const;

    // Increases size by one and returns reference
    // to new element
    template <typename T>
    T* extend();

    template <typename T>
    T* extend(t_uindex idx);

    void extend_dtype(t_uindex idx);

    // idx is in bytes
    template <typename T>
    T* get(t_uindex idx);

    // idx is in bytes
    template <typename T>
    const T* get(t_uindex idx) const;

    // idx is in items
    template <typename T>
    T* get_nth(t_uindex idx);

    // idx is in items
    template <typename T>
    const T* get_nth(t_uindex idx) const;

    // idx is in items
    const t_bool* get_nth_valid(t_uindex idx) const;

    // idx is in items
    t_bool* get_nth_valid(t_uindex idx);

    // idx is in items
    template <typename T>
    void set_nth(t_uindex idx, T v);

    template <typename T>
    void set_nth(t_uindex idx, T v, t_bool valid);

    void set_valid(t_uindex idx, t_bool valid);

    void set_size(t_uindex idx);

    void reserve(t_uindex idx);

    const t_lstore& data_lstore() const;

    t_uindex size() const;

    t_uindex get_vlenidx() const;

    const char* unintern_c(t_uindex idx) const;

    // Internal apis

    t_lstore* _get_data_lstore();

    t_vocab* _get_vocab();

    t_tscalar get_scalar(t_uindex idx) const;
    void set_scalar(t_uindex idx, t_tscalar value);

    t_bool is_valid_enabled() const;

    t_bool is_valid(t_uindex idx) const;

    t_bool is_vlen() const;

    void append(const t_column& other);

    void clear();

    template <typename VEC_T>
    void fill(VEC_T& vec,
              const t_uindex* bidx,
              const t_uindex* eidx) const;

    void pprint() const;

    template <typename DATA_T>
    void set_nth_body(t_uindex idx, DATA_T elem, bool valid);

    t_column_recipe get_recipe() const;

    // vocabulary must not contain empty string
    // indices should be > 0
    // scalars will be implicitly understood to be of dtype str
    template <typename VOCAB_T>
    void set_vocabulary(const VOCAB_T& vocab, size_t total_size = 0);

    void copy_vocabulary(const t_column* other);

    void pprint_vocabulary() const;

    template <typename DATA_T>
    void raw_fill(DATA_T v);

    t_col_sptr clone() const;

    t_col_sptr clone(const t_mask& mask) const;

    void valid_raw_fill(t_bool value);

    template <typename DATA_T>
    void copy_helper(const t_column* other,
                     const t_uidxvec& indices,
                     t_uindex offset);

    void copy(const t_column* other,
              const t_uidxvec& indices,
              t_uindex offset);

    void clear(t_uindex idx);

    void verify() const;
    void verify_size() const;
    void verify_size(t_uindex idx) const;

    t_uindex get_interned(const t_str& s);
    t_uindex get_interned(const char* s);
    void _rebuild_map();

    void borrow_vocabulary(const t_column& o);

  private:
    t_dtype m_dtype;
    t_bool m_init;
    t_bool m_isvlen;

    // For varlen columns this
    // contains t_uindex encoded ids
    t_lstore_sptr m_data;

    t_vocab_sptr m_vocab;

    // Missing value support
    t_lstore_sptr m_valid;

    t_uindex m_size;

    bool m_valid_enabled;

    t_bool m_from_recipe;

    t_uint32 m_elemsize;
};


typedef std::shared_ptr<t_column> t_col_sptr;
typedef std::shared_ptr<const t_column> t_col_csptr;
typedef std::vector<t_col_sptr> t_colsptrvec;
typedef std::vector<t_col_csptr> t_colcsptrvec;
typedef std::vector<t_col_sptr> t_colsptrvec;
typedef std::vector<t_col_csptr> t_colcsptrvec;

template <>
PERSPECTIVE_EXPORT void
t_column::push_back<const char*>(const char* elem);

template <>
PERSPECTIVE_EXPORT void t_column::push_back<char*>(char* elem);

template <>
PERSPECTIVE_EXPORT void
t_column::push_back<std::string>(std::string elem);

template <>
PERSPECTIVE_EXPORT void
t_column::push_back<t_tscalar>(t_tscalar elem);

template <>
PERSPECTIVE_EXPORT void
t_column::set_nth<const char*>(t_uindex idx, const char* elem);

template <>
PERSPECTIVE_EXPORT void t_column::set_nth<t_str>(t_uindex idx,
                                                 t_str elem);

template <>
PERSPECTIVE_EXPORT void t_column::set_nth<const char*>(
    t_uindex idx, const char* elem, t_bool valid);

template <>
PERSPECTIVE_EXPORT void
t_column::set_nth<t_str>(t_uindex idx, t_str elem, t_bool valid);

template <>
PERSPECTIVE_EXPORT const char*
t_column::get_nth<const char>(t_uindex idx) const;

template <typename T>
T*
t_column::get(t_uindex idx)
{
    return m_data->get<T>(idx);
}

template <typename T>
const T*
t_column::get(t_uindex idx) const
{
    return m_data->get<T>(idx);
}

template <typename T>
T*
t_column::get_nth(t_uindex idx)
{
    COLUMN_CHECK_ACCESS(idx);
    return m_data->get_nth<T>(idx);
}

template <typename T>
const T*
t_column::get_nth(t_uindex idx) const
{
    COLUMN_CHECK_ACCESS(idx);
    return m_data->get_nth<T>(idx);
}

template <typename T>
T*
t_column::extend()
{
    return extend<T>(1);
}

template <typename T>
T*
t_column::extend(t_uindex idx)
{
    T* rv = m_data->extend<T>(idx);
    m_size += idx;
    return rv;
}

template <typename DATA_T>
void
t_column::push_back(DATA_T elem)
{
    m_data->push_back(elem);
    ++m_size;
}

template <typename DATA_T>
void
t_column::push_back(DATA_T elem, t_bool valid)
{
    PSP_VERBOSE_ASSERT(is_valid_enabled(),
                       "Validity not enabled for column");
    m_data->push_back(elem);
    m_valid->push_back(valid);
    ++m_size;
}

// idx is in items

template <typename T>
void
t_column::set_nth(t_uindex idx, T v)
{
    COLUMN_CHECK_ACCESS(idx);
    m_data->set_nth<T>(idx, v);

    if (is_valid_enabled())
    {
        m_valid->set_nth<t_bool>(idx, true);
    }
}

template <typename T>
void
t_column::set_nth(t_uindex idx, T v, t_bool valid)
{
    COLUMN_CHECK_ACCESS(idx);
    m_data->set_nth<T>(idx, v);

    if (is_valid_enabled())
    {
        m_valid->set_nth<t_bool>(idx, valid);
    }
}

template <>
void t_column::fill<std::vector<const char*>>(
    std::vector<const char*>& vec,
    const t_uindex* bidx,
    const t_uindex* eidx) const;

template <typename VEC_T>
void
t_column::fill(VEC_T& vec,
               const t_uindex* bidx,
               const t_uindex* eidx) const
{

    PSP_VERBOSE_ASSERT(eidx - bidx > 0, "Invalid pointers passed in");

    for (t_uindex idx = 0, loop_end = eidx - bidx; idx < loop_end;
         ++idx)

    {
        vec[idx] =
            *(get_nth<typename VEC_T::value_type>(*(bidx + idx)));
    }
}

template <typename DATA_T>
void
t_column::set_nth_body(t_uindex idx, DATA_T elem, bool valid)
{
    COLUMN_CHECK_ACCESS(idx);
    PSP_VERBOSE_ASSERT(m_dtype == DTYPE_STR,
                       "Setting non string column");
    t_uindex interned = m_vocab->get_interned(elem);
    m_data->set_nth<t_uindex>(idx, interned);

    if (is_valid_enabled())
    {
        m_valid->set_nth<t_bool>(idx, valid);
    }
}

template <typename DATA_T>
void
t_column::raw_fill(DATA_T v)
{
    m_data->raw_fill(v);
}

template <typename VOCAB_T>
void
t_column::set_vocabulary(const VOCAB_T& vocab, size_t total_size)
{
    if( total_size )
        m_vocab->reserve( total_size, vocab.size() + 1 );

    for (const auto& kv : vocab)
        m_vocab->get_interned(kv.first.get_char_ptr());
}

template <>
void t_column::copy_helper<const char>(const t_column* other,
                                       const t_uidxvec& indices,
                                       t_uindex offset);

template <typename DATA_T>
void
t_column::copy_helper(const t_column* other,
                      const t_uidxvec& indices,
                      t_uindex offset)
{
    t_uindex eidx = std::min(other->size(),
                             static_cast<t_uindex>(indices.size()));
    reserve(eidx + offset);

    const DATA_T* o_base = other->get_nth<DATA_T>(0);
    DATA_T* base = get_nth<DATA_T>(0);

    for (t_uindex idx = 0; idx < eidx; ++idx)
    {
        base[idx + offset] = o_base[indices[idx]];
    }

    if (is_valid_enabled() && other->is_valid_enabled())
    {
        for (t_uindex idx = 0; idx < eidx; ++idx)
        {
            set_valid(idx + offset, other->is_valid(indices[idx]));
        }
    }
    COLUMN_CHECK_VALUES();
}

typedef std::vector<t_column> t_colvec;
typedef std::vector<t_column*> t_colptrvec;
typedef std::vector<const t_column*> t_colcptrvec;

inline void
scol_set(t_column* c, t_uindex row_idx, const char* s)
{
    c->set_nth<const char*>(row_idx, s);
}

} // end namespace perspective
