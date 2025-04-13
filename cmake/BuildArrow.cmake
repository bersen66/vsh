message(STATUS "setting up Apache.Arrow")
include(ExternalProject)

ExternalProject_Add(
    apache_arrow_submodule
    SOURCE_DIR "${THIRD_PARTY_DIR}/arrow"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/arrow/build"
    INSTALL_DIR "${EXTERN_DIR}/arrow"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX="${EXTERN_DIR}/arrow/"
)
