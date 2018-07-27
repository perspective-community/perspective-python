cmake_minimum_required (VERSION 2.8)
project (psp)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AR ar)
set(CMAKE_RANLIB ranlib)
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
list(APPEND CMAKE_PREFIX_PATH /usr/local)

include_directories("${CMAKE_SOURCE_DIR}/perspective/include")
include_directories("/usr/local/include")
include_directories("/usr/local/include/python3.7m")
include_directories("/boost_includes")


if(DEFINED ENV{PSP_DEBUG})
	set(OPT_FLAGS "-O1 -g4 -s ASSERTIONS=2")
else()
    set(OPT_FLAGS "-O3 -g0")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTENDED_FLAGS} ${OPT_FLAGS} ")

set (SOURCE_FILES
perspective/src/aggregate.cpp
perspective/src/aggspec.cpp
perspective/src/arg_sort.cpp
perspective/src/base.cpp
perspective/src/base_impl_linux.cpp
perspective/src/base_impl_osx.cpp
perspective/src/base_impl_win.cpp
perspective/src/build_filter.cpp
perspective/src/calc_agg_dtype.cpp
perspective/src/column.cpp
perspective/src/comparators.cpp
perspective/src/compat.cpp
perspective/src/compat_impl_linux.cpp
perspective/src/compat_impl_osx.cpp
perspective/src/compat_impl_win.cpp
perspective/src/config.cpp
perspective/src/context_base.cpp
perspective/src/context_grouped_pkey.cpp
perspective/src/context_handle.cpp
perspective/src/context_one.cpp
perspective/src/context_two.cpp
perspective/src/context_zero.cpp
perspective/src/custom_column.cpp
perspective/src/date.cpp
perspective/src/dense_nodes.cpp
perspective/src/dense_tree_context.cpp
perspective/src/dense_tree.cpp
perspective/src/dependency.cpp
perspective/src/extract_aggregate.cpp
perspective/src/filter.cpp
perspective/src/flat_traversal.cpp
perspective/src/gnode.cpp
perspective/src/gnode_state.cpp
perspective/src/histogram.cpp
perspective/src/logtime.cpp
perspective/src/mask.cpp
perspective/src/min_max.cpp
perspective/src/multi_sort.cpp
perspective/src/none.cpp
perspective/src/path.cpp
perspective/src/pivot.cpp
perspective/src/pool.cpp
perspective/src/port.cpp
perspective/src/query.cpp
perspective/src/raii.cpp
perspective/src/raii_impl_linux.cpp
perspective/src/raii_impl_osx.cpp
perspective/src/raii_impl_win.cpp
perspective/src/range.cpp
perspective/src/rlookup.cpp
perspective/src/scalar.cpp
perspective/src/schema_column.cpp
perspective/src/schema.cpp
perspective/src/slice.cpp
perspective/src/sort_specification.cpp
perspective/src/sparse_tree.cpp
perspective/src/sparse_tree_node.cpp
perspective/src/step_delta.cpp
perspective/src/storage.cpp
perspective/src/storage_impl_linux.cpp
perspective/src/storage_impl_osx.cpp
perspective/src/storage_impl_win.cpp
perspective/src/sym_table.cpp
perspective/src/table.cpp
perspective/src/time.cpp
perspective/src/traversal.cpp
perspective/src/traversal_nodes.cpp
perspective/src/tree_context_common.cpp
perspective/src/utils.cpp
perspective/src/update_task.cpp
perspective/src/vocab.cpp
)

set(CMAKE_CXX_FLAGS " -std=c++0x ${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

add_library(psp.so ${SOURCE_FILES})
