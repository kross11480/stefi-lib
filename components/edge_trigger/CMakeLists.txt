add_library(EdgeTrigger STATIC edge_trigger.c)
add_library(stefi::edge_trigger ALIAS EdgeTrigger)

target_include_directories(EdgeTrigger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EdgeTrigger PUBLIC board_iface libstefi)
