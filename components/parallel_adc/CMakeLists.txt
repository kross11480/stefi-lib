add_library(ParallelAdc STATIC parallel_adc.c)
add_library(stefi::parallel_adc ALIAS ParallelAdc)

# F7: stream DMA (double buffer); L4/G4: channel DMA (single transfer).
target_include_directories(ParallelAdc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ParallelAdc PUBLIC board_iface libstefi)
