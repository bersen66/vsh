
add_subdirectory(vsh)

if (VSH_BUILD_DRIVER) 
    add_subdirectory(driver)
endif()
