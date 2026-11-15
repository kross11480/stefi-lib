add_library(CmdLine STATIC cmd_line.c)
add_library(stefi::cmd_line ALIAS CmdLine)

# Pure C, no hardware: portable to every board.
target_include_directories(CmdLine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CmdLine PUBLIC libstefi)
