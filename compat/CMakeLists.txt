add_library(implus_compat INTERFACE)
target_link_libraries(implus_compat INTERFACE range-v3)

target_include_directories(implus_compat INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})