set(
    VSH_SOURCES
    src/vsh.cpp 
)

add_library(vsh ${VSH_SOURCES})
target_include_directories(
    vsh PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
