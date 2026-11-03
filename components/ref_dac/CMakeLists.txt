add_library(RefDac STATIC ref_dac.c)
add_library(stefi::ref_dac ALIAS RefDac)

target_include_directories(RefDac PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RefDac PUBLIC board_iface libstefi)
