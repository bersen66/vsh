message(STATUS "setting up Apache.Datasketches")

set(DATASKETCHES_SOURCES_DIR "${THIRD_PARTY_DIR}/datasketches/")
set(DATASKETCHES_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps/datasketches/")
set(DATASKETCHES_INSTALL_DIR "${EXTERN_DIR}/datasketches/")


execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${DATASKETCHES_SOURCES_DIR}
        -B ${DATASKETCHES_BUILD_DIR}
        -DCMAKE_INSTALL_PREFIX=${DATASKETCHES_INSTALL_DIR}
        -DBUILD_TESTS=OFF
    RESULT_VARIABLE result
)


if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure Datasketches: ${result}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${DATASKETCHES_BUILD_DIR} -j 8 --target install
    RESULT_VARIABLE result
)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure Datasketches: ${result}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${DATASKETCHES_INSTALL_DIR}/lib/DataSketches/cmake/")

find_package(DataSketches REQUIRED)
