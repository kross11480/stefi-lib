add_executable(hl_blink hl_blink.c)

target_link_libraries(hl_blink PRIVATE stefi::board stefi::simpleio stefi::oled)
