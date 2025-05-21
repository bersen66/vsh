message(STATUS "setting up indicators")

set(INDICATORS_SOURCES_DIR "${THIRD_PARTY_DIR}/indicators/")
set(INDICATORS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps/indicators/")
set(INDICATORS_INSTALL_DIR "${EXTERN_DIR}/indicators")

if (NOT EXISTS "${EXTERN_DIR}/indicators/lib/cmake")
    message(STATUS "Configuring indicators")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S ${INDICATORS_SOURCES_DIR}
            -B ${INDICATORS_BUILD_DIR}
            -DCMAKE_INSTALL_PREFIX=${INDICATORS_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DINDICATORS_BUILD_TESTS=OFF
            -DINDICATORS_SAMPLES=OFF
            -DINDICATORS_DEMO=OFF
        RESULT_VARIABLE result
    )

    if (NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to configure Indicators: ${result}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${INDICATORS_BUILD_DIR} -j 8 --target install
        RESULT_VARIABLE result
    )

    if (NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to compile and install Indicators: ${result}")
    endif()

else()
    message(STATUS "Indicators installed at ${INDICATORS_INSTALL_DIR}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${INDICATORS_INSTALL_DIR}/lib/cmake/indicators")
find_package(indicators REQUIRED)
