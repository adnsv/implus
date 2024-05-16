add_library(implus_lbrk STATIC "lbrk.cpp")
add_library(implus::lbrk ALIAS implus_lbrk)

target_include_directories(implus_lbrk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

option(IMPLUS_LBRK_TESTS "ImPlus: build with LBRK tests" OFF)

if(IMPLUS_LBRK_TESTS)
    add_subdirectory(tests)
endif()