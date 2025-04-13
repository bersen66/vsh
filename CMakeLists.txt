cmake_minimum_required(VERSION 3.16)
project(vsh)

set(VSH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(THIRD_PARTY_DIR "${VSH_ROOT}/third_party/")
set(EXTERN_DIR "${VSH_ROOT}/extern/")

list(APPEND CMAKE_MODULE_PATH "${VSH_ROOT}/cmake")

include(PrepareExternDir)
include(BuildArrow)
# add_subdirectory(lib)

