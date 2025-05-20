set(SANTIZER_VALUES "ub" "leak" "address" "thread" "none")

set(VSH_USE_SANITIZER "none"
    CACHE STRING "Enable sanitizers: ${SANTIZER_VALUES}")
set_property(CACHE VSH_USE_SANITIZER PROPERTY STRINGS ${SANTIZER_VALUES})

if (NOT VSH_USE_SANITIZER  IN_LIST SANTIZER_VALUES)
    message(FATAL_ERROR "Invalid sanitizer: ${VSH_USE_SANITIZER}")
endif()


if (VSH_USE_SANITIZER STREQUAL "ub")
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
elseif (VSH_USE_SANITIZER STREQUAL "leak")
    add_compile_options(-fsanitize=leak)
    add_link_options(-fsanitize=leak)
elseif (VSH_USE_SANITIZER STREQUAL "address")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
elseif (VSH_USE_SANITIZER STREQUAL "thread")
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
