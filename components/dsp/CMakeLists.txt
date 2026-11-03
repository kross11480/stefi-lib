add_library(Dsp STATIC moving_average.c)
add_library(stefi::dsp ALIAS Dsp)

# Pure C, no hardware: portable to every board.
target_include_directories(Dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Dsp PUBLIC libstefi)
