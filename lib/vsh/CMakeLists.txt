set(
    VSH_SOURCES
    src/internal/parquet_key_iterator.cpp
    src/key_iterator.cpp 
    src/eh_sketch.cpp
    src/histogram.cpp
    src/quantile_hist.cpp
    src/bar_splitting_hist.cpp
)

add_library(vsh ${VSH_SOURCES})
target_include_directories(
    vsh PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    vsh
    Arrow::arrow_static
    Parquet::parquet_static
    datasketches
)


if (VSH_BUILD_TESTS)
    set(
        VSH_TESTING_SOURCES
        tests/eh_sketch_test.cpp
        tests/bar_splitting_hist_test.cpp
    )
    
    add_executable(vsh_tests ${VSH_TESTING_SOURCES})
    target_link_libraries(vsh_tests vsh GTest::gtest_main)
    gtest_discover_tests(vsh_tests)
endif()




