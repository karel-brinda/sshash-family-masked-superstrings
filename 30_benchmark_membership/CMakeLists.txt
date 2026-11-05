cmake_minimum_required(VERSION 3.16)
project(BENCHMARK_MEMBERSHIP LANGUAGES CXX)

# Every tool of the SSHash family is compiled here from its submodule sources with the same
# compiler and flags, so that 0x vs 1x differences reflect code, not build settings.
#
#   sshash-lite-01, sshash-lite-11   build/query CLIs of 01_sshash-lite / 11_sshash-lite-bigreedy-ms
#   sshash-02, sshash-12             build/query CLIs of 02_sshash / 12_sshash-bigreedy-ms
#   bench-01, bench-11               src/sshash_lite_benchmark.cpp (port of the reference harness)
#   bench-02, bench-12               00_kmer_sets_benchmark/src/sshash_benchmark.cpp, unchanged
#
# The Rust tools (03, 13) are built by rust/ (see README.md).

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

# --- SSHash (02, 12) ---------------------------------------------------------------------------

function(add_sshash id dir)
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

    add_library(sshash_static_${id} STATIC
        ${src}/external/gz/zip_stream.cpp
        ${src}/external/cityhash/cityhash.cpp
        ${src}/src/builder/build.cpp
        ${src}/src/dictionary.cpp
        ${src}/src/query.cpp
        ${src}/src/info.cpp)
    target_include_directories(sshash_static_${id} PUBLIC ${includes})
    target_compile_features(sshash_static_${id} PUBLIC cxx_std_17)
    target_link_libraries(sshash_static_${id} PUBLIC ZLIB::ZLIB)

    add_executable(sshash-${id} ${src}/tools/sshash.cpp)
    target_link_libraries(sshash-${id} PRIVATE sshash_static_${id})

    # The reference harness includes "sshash/...": expose this tree under that name.
    set(alias ${CMAKE_BINARY_DIR}/alias_${id})
    file(MAKE_DIRECTORY ${alias})
    file(CREATE_LINK ${src} ${alias}/sshash SYMBOLIC)

    add_executable(bench-${id} src/sshash_benchmark.cpp)
    target_include_directories(bench-${id} PRIVATE ${alias} ${REF})
    target_link_libraries(bench-${id} PRIVATE sshash_static_${id})
endfunction()

add_sshash(02 02_sshash)
add_sshash(12 12_sshash-bigreedy-ms)

# --- SSHash-Lite (01, 11) ----------------------------------------------------------------------

# 01 is built from a copy patched with the PTHash stabilization of 11 (single-threaded, fixed
# seed): without it 01 does not build reliably on macOS/ARM, and with it 01 and 11 differ only in
# the masked-superstring feature. See patches/01-pthash-stabilization.patch.
option(BENCH_STABILIZE_01 "Build 01 with the PTHash stabilization of 11" ON)
if (BENCH_STABILIZE_01)
    set(SRC01 ${CMAKE_BINARY_DIR}/01_sshash-lite-stabilized)
    set(PATCH01 ${CMAKE_CURRENT_SOURCE_DIR}/patches/01-pthash-stabilization.patch)
    if (NOT EXISTS ${SRC01}/.patched OR ${PATCH01} IS_NEWER_THAN ${SRC01}/.patched)
        file(REMOVE_RECURSE ${SRC01})
        file(MAKE_DIRECTORY ${SRC01})
        file(COPY ${ROOT}/01_sshash-lite/include ${ROOT}/01_sshash-lite/src DESTINATION ${SRC01})
        file(CREATE_LINK ${ROOT}/01_sshash-lite/external ${SRC01}/external SYMBOLIC)
        execute_process(COMMAND patch -p1 -i ${PATCH01} WORKING_DIRECTORY ${SRC01}
                        RESULT_VARIABLE rc)
        if (NOT rc EQUAL 0)
            message(FATAL_ERROR "cannot apply ${PATCH01}")
        endif ()
        file(TOUCH ${SRC01}/.patched)
    endif ()
else ()
    set(SRC01 ${ROOT}/01_sshash-lite)
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

add_sshash_lite(01 ${SRC01} sshash-lite 20)  # C++20: std::popcount in PTHash on non-x86
add_sshash_lite(11 ${ROOT}/11_sshash-lite-bigreedy-ms sshash-lite-bigreedy-ms 20)
