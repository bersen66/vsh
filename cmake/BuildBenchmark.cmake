message(STATUS "setting up Google.benchmark")

set(BENCHMARK_SOURCES_DIR "${THIRD_PARTY_DIR}/gbench/")
set(BENCHMARK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps/gbench/")
set(BENCHMARK_INSTALL_DIR "${EXTERN_DIR}/benchmark")


if (NOT EXISTS "${EXTERN_DIR}/benchmark/lib/cmake")

    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S ${BENCHMARK_SOURCES_DIR}
            -B ${BENCHMARK_BUILD_DIR}
            -DCMAKE_INSTALL_PREFIX=${BENCHMARK_INSTALL_DIR}
            -DBENCHMARK_ENABLE_TESTING=OFF
            -DCMAKE_BUILD_TYPE=Release
        RESULT_VARIABLE result
    )

    if (NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to configure Google.benchmark ${result}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${BENCHMARK_BUILD_DIR} -j 8 --target install
        RESULT_VARIABLE result
    )

    if (NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to compile and install Google.benchmark: ${result}")
    endif()

else()
    message(STATUS "Google.benchmark installed at ${GTEST_INSTALL_DIR}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${BENCHMARK_INSTALL_DIR}/lib/cmake/benchmark/")
find_package(benchmark REQUIRED)
