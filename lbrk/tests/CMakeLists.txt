set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(lbrk_test main.cpp test_lbrk.cpp)
target_link_libraries(lbrk_test implus::lbrk)
