add_library(ParallelAdc STATIC parallel_adc.c)
add_library(stefi::parallel_adc ALIAS ParallelAdc)

# Needs the stream DMA's double-buffer mode: STM32F7 boards only.
target_include_directories(ParallelAdc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ParallelAdc PUBLIC board_iface libstefi)
