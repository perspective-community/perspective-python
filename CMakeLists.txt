cmake_minimum_required (VERSION 2.8)
project (psp)

if(DEFINED ENV{use_jpm})
    add_definitions(-DPSP_ENABLE_PYTHON_JPM)
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_AR ar)
    set(CMAKE_RANLIB ranlib)
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
elseif(DEFINED ENV{use_ems})
    add_definitions(-DPSP_ENABLE_WASM)
    set(CMAKE_C_COMPILER emcc)
    set(CMAKE_CXX_COMPILER em++)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
    set(CMAKE_AR emar)
    set(CMAKE_RANLIB emranlib)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
else()
    add_definitions(-DPSP_ENABLE_PYTHON)
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_AR ar)
    set(CMAKE_RANLIB ranlib)
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
endif()

list(APPEND CMAKE_PREFIX_PATH /usr/local)

include_directories("${CMAKE_SOURCE_DIR}/perspective/include")
include_directories("/usr/local/include")
include_directories("/usr/local/include/python3.7m")
include_directories("/boost_includes")


find_package(TBB)
find_package( Boost COMPONENTS python REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

find_package( PythonLibs 3.7 REQUIRED )
include_directories( ${PYTHON_INCLUDE_DIRS} )


if(DEFINED ENV{use_ems})
    set(EXTENDED_FLAGS " \
        --bind \
        --memory-init-file 1 \
        -s NO_EXIT_RUNTIME=1 \
        -s NO_FILESYSTEM=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s NO_DYNAMIC_EXECUTION=2 \
        -s EXPORTED_FUNCTIONS=\"['_main']\" \
    ")

    if(DEFINED ENV{PSP_DEBUG})
        set(OPT_FLAGS " \
            -O1 \
            -g4 \
            -s SAFE_HEAP=1 \
            -s DISABLE_EXCEPTION_CATCHING=0 \
            -s ASSERTIONS=2 \
            -s DEMANGLE_SUPPORT=1 \
        ")
    else()
        set(OPT_FLAGS " \
            -O3 \
            -g0 \
        ")
    endif()

    set(SYNC_MODE_FLAGS "-s BINARYEN_ASYNC_COMPILATION=0 -s BINARYEN_METHOD='\"native-wasm\"' -s WASM=1")
    
    set(ASYNC_MODE_FLAGS "-s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_METHOD='\"native-wasm\"' -s WASM=1")
    
    set(ASMJS_MODE_FLAGS "-s BINARYEN_ASYNC_COMPILATION=0 -s BINARYEN_METHOD='\"asmjs\"' -Wno-almost-asm")

else()
    if(DEFINED ENV{PSP_DEBUG})
    	set(OPT_FLAGS "-O1 -g4 -s ASSERTIONS=2 -DPSP_ENABLE_PYTHON")
    else()
        set(OPT_FLAGS "-O3 -g0 -DPSP_ENABLE_PYTHON")
    endif()
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
perspective/src/chart_interfaces.cpp
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

if(DEFINED ENV{use_ems})
    add_library(psp ${SOURCE_FILES})
    set_target_properties(psp PROPERTIES COMPILE_FLAGS "${ASYNC_MODE_FLAGS}")

    add_executable(perspective.async perspective/src/main_js.cpp)
    target_link_libraries(perspective.async psp "${ASYNC_MODE_FLAGS}")
    set_target_properties(perspective.async PROPERTIES COMPILE_FLAGS "${ASYNC_MODE_FLAGS}")
    set_target_properties(perspective.async PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../build/wasm_async/")
    set_target_properties(perspective.async PROPERTIES OUTPUT_NAME "psp")

    add_executable(perspective.sync perspective/src/main_js.cpp)
    target_link_libraries(perspective.sync psp "${SYNC_MODE_FLAGS}")
    set_target_properties(perspective.sync PROPERTIES COMPILE_FLAGS "${SYNC_MODE_FLAGS}")
    set_target_properties(perspective.sync PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../build/wasm_sync/")
    set_target_properties(perspective.sync PROPERTIES OUTPUT_NAME "psp")
    add_dependencies(perspective.sync perspective.async)

    add_executable(perspective.asm perspective/src/main_js.cpp)
    target_link_libraries(perspective.asm psp "${ASMJS_MODE_FLAGS}")
    set_target_properties(perspective.asm PROPERTIES COMPILE_FLAGS "${ASMJS_MODE_FLAGS}")
    set_target_properties(perspective.asm PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../build/asmjs/")
    set_target_properties(perspective.asm PROPERTIES OUTPUT_NAME "psp")
    add_dependencies(perspective.asm perspective.sync)
else()
    add_library(psp SHARED ${SOURCE_FILES})
    target_link_libraries(psp tbb)

    add_library(binding SHARED perspective/src/binding.cpp)
    target_link_libraries(binding psp)
    target_link_libraries(binding tbb)
    target_link_libraries(binding ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

    add_executable(perspective_test perspective/src/main.cpp)
    target_link_libraries(perspective_test psp)
    target_link_libraries(perspective_test tbb)

    if(UNIX AND NOT APPLE)
        target_link_libraries(perspective_test pthread)
    endif()

    if(APPLE)
        set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
    endif(APPLE)


    set_target_properties(perspective_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../")

endif()