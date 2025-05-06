cmake_minimum_required(VERSION 3.16)
project(vsh)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VSH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(THIRD_PARTY_DIR "${VSH_ROOT}/third_party/")
set(EXTERN_DIR "${VSH_ROOT}/extern/")

list(APPEND CMAKE_MODULE_PATH "${VSH_ROOT}/cmake")
include(VshOptions)
include(PrepareExternDir)
include(BuildArrow)
include(BuildDatasketches)

if (VSH_BUILD_TESTS)
    include(BuildGtest)
    enable_testing()
    include(GoogleTest)
endif()

add_subdirectory(lib)

