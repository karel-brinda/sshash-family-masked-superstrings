cmake_minimum_required(VERSION 3.16)
project(BENCHMARK_MEMBERSHIP LANGUAGES CXX)

# Every tool of the SSHash family is compiled here from its submodule sources with the same
# compiler and flags, so that 0x vs 1x differences reflect code, not build settings.
#
#   sshash-01-*, sshash-11-*           build/query CLIs of 01_sshash-v3 / 11_sshash-v3-bigreedy-ms
#   sshash-lite-02, sshash-lite-12     build/query CLIs of 02_sshash-lite / 12_sshash-lite-bigreedy-ms
#   sshash-03, sshash-13               CLI of 03_sshash-v4 / 13_sshash-v4-bigreedy-ms
#   sshash-04, sshash-14               CLI of 04_sshash-v6 / 14_sshash-v6-bigreedy-ms (SSHash v6)
#   bench-01, bench-11                 src/sshash_v3_benchmark.cpp (port of the reference harness)
#   bench-02, bench-12                 src/sshash_lite_benchmark.cpp (port of the reference harness)
#   bench-03, bench-13                 src/sshash_v4_benchmark.cpp (reference harness, v4 API)
#   bench-04, bench-14                 src/sshash_benchmark.cpp (reference harness, v6 API)
#
# The Rust tools (05, 15) are built by rust/ (see README.md).

set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    add_compile_options(-march=native -mbmi2 -mavx2 -msse4.2)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    add_compile_options(-mcpu=native)
endif ()
add_compile_options(-ggdb -pthread -Wno-deprecated-declarations)
add_link_options(-pthread)

find_package(ZLIB REQUIRED)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(REF ${ROOT}/00_kmer_sets_benchmark)

# --- SSHash v6 (04, 14) and v4 (03, 13) -------------------------------------------------------

# add_sshash(<id> <dir> <harness> <library sources...>): the two versions differ only in their
# source layout and in the API adaptations of their harness.
function(add_sshash id dir harness)
    set(src ${ROOT}/${dir})
    set(includes
        ${src}/include
        ${src}
        ${src}/external/pthash/include
        ${src}/external/pthash/external/bits/include
        ${src}/external/pthash/external/fastmod
        ${src}/external/pthash/external/bits/external/essentials/include
        ${src}/external/pthash/external/xxHash
        ${src}/external/pthash/external/mm_file/include)

    list(TRANSFORM ARGN PREPEND ${src}/ OUTPUT_VARIABLE sources)
    add_library(sshash_static_${id} STATIC ${sources})
    target_include_directories(sshash_static_${id} PUBLIC ${includes})
    target_compile_features(sshash_static_${id} PUBLIC cxx_std_17)
    target_link_libraries(sshash_static_${id} PUBLIC ZLIB::ZLIB)

    add_executable(sshash-${id} ${src}/tools/sshash.cpp)
    target_link_libraries(sshash-${id} PRIVATE sshash_static_${id})

    # The harnesses include "sshash/...": expose this tree under that name.
    set(alias ${CMAKE_BINARY_DIR}/alias_${id})
    file(MAKE_DIRECTORY ${alias})
    file(CREATE_LINK ${src} ${alias}/sshash SYMBOLIC)

    add_executable(bench-${id} ${harness})
    target_include_directories(bench-${id} PRIVATE ${alias} ${REF})
    target_link_libraries(bench-${id} PRIVATE sshash_static_${id})
endfunction()

set(SOURCES_V6 external/gz/zip_stream.cpp external/cityhash/cityhash.cpp src/builder/build.cpp
               src/dictionary.cpp src/query.cpp src/info.cpp)
set(SOURCES_V4 external/gz/zip_stream.cpp src/build.cpp src/dictionary.cpp src/query.cpp
               src/info.cpp src/statistics.cpp)
add_sshash(04 04_sshash-v6 src/sshash_benchmark.cpp ${SOURCES_V6})
add_sshash(14 14_sshash-v6-bigreedy-ms src/sshash_benchmark.cpp ${SOURCES_V6})
add_sshash(03 03_sshash-v4 src/sshash_v4_benchmark.cpp ${SOURCES_V4})
add_sshash(13 13_sshash-v4-bigreedy-ms src/sshash_v4_benchmark.cpp ${SOURCES_V4})

# --- SSHash v3 (01, 11) ------------------------------------------------------------------------

# v3.0.0 (2022) needs two fixes to build and run on macOS/ARM, carried as commits by 11 and applied
# here to a build-time copy of 01 (the submodule is untouched), so that 01 and 11 differ only in
# the masked-superstring feature:
#   patches/pthash-portability.patch   PTHash e12ce2f..5da0377 (non-x86 fallbacks)
#   patches/pthash-stabilization.patch the PTHash stabilization of 02/12 (v3 and SSHash-Lite
#                                         share this code; the patch applies verbatim)
# and C++20 (std::popcount in the PTHash fallbacks, as for SSHash-Lite).
set(SRC_V3 ${CMAKE_BINARY_DIR}/sshash-v3-patched)
set(PATCHES_V3 ${CMAKE_CURRENT_SOURCE_DIR}/patches/pthash-portability.patch
              ${CMAKE_CURRENT_SOURCE_DIR}/patches/pthash-stabilization.patch)
set(STALE_V3 FALSE)
foreach (patch ${PATCHES_V3})
    if (NOT EXISTS ${SRC_V3}/.patched OR ${patch} IS_NEWER_THAN ${SRC_V3}/.patched)
        set(STALE_V3 TRUE)
    endif ()
endforeach ()
if (STALE_V3)
    file(REMOVE_RECURSE ${SRC_V3})
    file(MAKE_DIRECTORY ${SRC_V3})
    file(COPY ${ROOT}/01_sshash-v3/include ${ROOT}/01_sshash-v3/src ${ROOT}/01_sshash-v3/external
         DESTINATION ${SRC_V3} PATTERN .git EXCLUDE)
    foreach (patch ${PATCHES_V3})
        execute_process(COMMAND patch -p1 -i ${patch} WORKING_DIRECTORY ${SRC_V3} RESULT_VARIABLE rc)
        if (NOT rc EQUAL 0)
            message(FATAL_ERROR "cannot apply ${patch}")
        endif ()
    endforeach ()
    file(TOUCH ${SRC_V3}/.patched)
endif ()

function(add_sshash_v3 id src)
    add_library(sshash_static_${id} STATIC
        ${src}/include/gz/zip_stream.cpp
        ${src}/include/dictionary.cpp
        ${src}/include/info.cpp
        ${src}/include/builder/build.cpp)
    target_compile_features(sshash_static_${id} PUBLIC cxx_std_20)
    target_link_libraries(sshash_static_${id} PUBLIC ZLIB::ZLIB)

    foreach (cli build query)
        add_executable(sshash-${id}-${cli} ${src}/src/${cli}.cpp)
        target_link_libraries(sshash-${id}-${cli} PRIVATE sshash_static_${id})
    endforeach ()

    add_executable(bench-${id} src/sshash_v3_benchmark.cpp)
    target_include_directories(bench-${id} PRIVATE ${src})
    target_link_libraries(bench-${id} PRIVATE sshash_static_${id})
endfunction()

add_sshash_v3(01 ${SRC_V3})
add_sshash_v3(11 ${ROOT}/11_sshash-v3-bigreedy-ms)

# --- SSHash-Lite (02, 12) ----------------------------------------------------------------------

# 02 is built from a copy patched with the PTHash stabilization of 12 (single-threaded, fixed
# seed): without it 02 does not build reliably on macOS/ARM, and with it 02 and 12 differ only in
# the masked-superstring feature. See patches/pthash-stabilization.patch.
option(BENCH_STABILIZE_LITE "Build 02 with the PTHash stabilization of 12" ON)
if (BENCH_STABILIZE_LITE)
    set(SRC_LITE ${CMAKE_BINARY_DIR}/sshash-lite-stabilized)
    set(PATCH_LITE ${CMAKE_CURRENT_SOURCE_DIR}/patches/pthash-stabilization.patch)
    if (NOT EXISTS ${SRC_LITE}/.patched OR ${PATCH_LITE} IS_NEWER_THAN ${SRC_LITE}/.patched)
        file(REMOVE_RECURSE ${SRC_LITE})
        file(MAKE_DIRECTORY ${SRC_LITE})
        file(COPY ${ROOT}/02_sshash-lite/include ${ROOT}/02_sshash-lite/src DESTINATION ${SRC_LITE})
        file(CREATE_LINK ${ROOT}/02_sshash-lite/external ${SRC_LITE}/external SYMBOLIC)
        execute_process(COMMAND patch -p1 -i ${PATCH_LITE} WORKING_DIRECTORY ${SRC_LITE}
                        RESULT_VARIABLE rc)
        if (NOT rc EQUAL 0)
            message(FATAL_ERROR "cannot apply ${PATCH_LITE}")
        endif ()
        file(TOUCH ${SRC_LITE}/.patched)
    endif ()
else ()
    set(SRC_LITE ${ROOT}/02_sshash-lite)
endif ()

function(add_sshash_lite id src cli std)
    add_library(sshashlite_static_${id} STATIC
        ${src}/include/gz/zip_stream.cpp
        ${src}/include/dictionary.cpp
        ${src}/include/info.cpp
        ${src}/include/builder/build.cpp)
    target_compile_features(sshashlite_static_${id} PUBLIC cxx_std_${std})
    target_include_directories(sshashlite_static_${id} PUBLIC ${src}/external/pthash/external/essentials/include)
    target_link_libraries(sshashlite_static_${id} PUBLIC ZLIB::ZLIB)

    add_executable(sshash-lite-${id} ${src}/src/${cli}.cpp)
    target_link_libraries(sshash-lite-${id} PRIVATE sshashlite_static_${id})

    add_executable(bench-${id} src/sshash_lite_benchmark.cpp)
    target_include_directories(bench-${id} PRIVATE ${src})
    target_link_libraries(bench-${id} PRIVATE sshashlite_static_${id})
endfunction()

add_sshash_lite(02 ${SRC_LITE} sshash-lite 20)  # C++20: std::popcount in PTHash on non-x86
add_sshash_lite(12 ${ROOT}/12_sshash-lite-bigreedy-ms sshash-lite-bigreedy-ms 20)
