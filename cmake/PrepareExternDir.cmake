if (NOT EXISTS "${EXTERN_DIR}")
    message(STATUS "Extern dir missed, creating it")
    file(MAKE_DIRECTORY "${EXTERN_DIR}")
else() 
    message(STATUS "Extern dir exists, skip creation")
endif()

