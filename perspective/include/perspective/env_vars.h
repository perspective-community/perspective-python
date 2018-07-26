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
#include <perspective/exports.h>
#include <cstdlib>

namespace perspective
{

struct PERSPECTIVE_EXPORT t_env
{
    static inline t_bool
    log_time()
    {
        static const t_bool rv = std::getenv("PSP_LOG_TIME") != 0;
        return rv;
    }

    static inline t_bool
    log_storage_resize()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_STORAGE_RESIZE") != 0;
        return rv;
    }

    static inline t_bool
    log_schema_gnode_flattened()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_SCHEMA_GNODE_FLATTENED") != 0;
        return rv;
    }

    static inline t_bool
    log_data_pool_send()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_POOL_SEND") != 0;
        return rv;
    }

    static inline t_bool
    log_time_gnode_process()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_TIME_GNODE_PROCESS") != 0;
        return rv;
    }

    static inline t_bool
    log_time_ctx_notify()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_TIME_CTX_NOTIFY") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_flattened()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_FLATTENED") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_delta()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_DELTA") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_prev()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_PREV") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_current()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_CURRENT") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_transitions()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_TRANSITIONS") != 0;
        return rv;
    }

    static inline t_bool
    log_data_gnode_existed()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_GNODE_EXISTED") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_strands()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_STRANDS") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_strand_deltas()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_STRAND_DELTAS") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_dtree()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_DTREE") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_dctx()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_DCTX") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_stree_prev()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_STREE_PREV") != 0;
        return rv;
    }

    static inline t_bool
    log_data_nsparse_stree_after()
    {
        static const t_bool rv =
            std::getenv("PSP_LOG_DATA_NSPARSE_STREE_AFTER") != 0;
        return rv;
    }

    static inline t_bool
    log_progress()
    {
        static const t_bool rv = std::getenv("PSP_LOG_PROGRESS") != 0;
        return rv;
    }

    static inline t_bool
    show_svg_browser()
    {
        static const t_bool rv =
            std::getenv("PSP_SHOW_SVG_BROWSER") != 0;
        return rv;
    }

    static inline t_bool
    backout_nveq_ft()
    {
        static const t_bool rv =
            std::getenv("PSP_BACKOUT_NVEQ_FT") != 0;
        return rv;
    }

    static inline t_bool
    backout_invalid_neq_ft()
    {
        static const t_bool rv =
            std::getenv("PSP_BACKOUT_INVALID_NEQ_FT") != 0;
        return rv;
    }

    static inline t_bool
    backout_force_current_row()
    {
        static const t_bool rv =
            std::getenv("PSP_BACKOUT_FORCE_CURRENT_ROW") != 0;
        return rv;
    }

    static inline t_bool
    backout_eq_invalid_invalid()
    {
        static const t_bool rv =
            std::getenv("PSP_BACKOUT_EQ_INVALID_INVALID") != 0;
        return rv;
    }
};

} // end namespace perspective
