add_library(SimpleIO STATIC
        pin.c
        led.c
        button.c
        hardware_timer.c
)
add_library(stefi::simpleio ALIAS SimpleIO)

target_include_directories(SimpleIO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SimpleIO PUBLIC board_iface libstefi)
