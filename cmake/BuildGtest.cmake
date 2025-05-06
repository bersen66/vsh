message(STATUS "setting up Google.Test")

set(GTEST_SOURCES_DIR "${THIRD_PARTY_DIR}/gtest/")
set(GTEST_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps/gtest")
set(GTEST_INSTALL_DIR "${EXTERN_DIR}/gtest")


execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${GTEST_SOURCES_DIR}
        -B ${GTEST_BUILD_DIR}
        -DCMAKE_INSTALL_PREFIX=${GTEST_INSTALL_DIR}
    RESULT_VARIABLE result
)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure Google.Test: ${result}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${GTEST_BUILD_DIR} -j 8 --target install
    RESULT_VARIABLE result
)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to compile and install Google.Test: ${result}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${EXTERN_DIR}/gtest/lib/cmake/GTest/")

find_package(GTest REQUIRED)


