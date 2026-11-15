add_library(EventFsm STATIC event_fsm.c)
add_library(stefi::fsm ALIAS EventFsm)

# Portable: only needs the HAL ringbuffer.
target_include_directories(EventFsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EventFsm PUBLIC libstefi)
