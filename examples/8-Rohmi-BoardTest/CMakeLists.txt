# Rohmi-only: the examples glob also visits this directory when building
# for nucleo_l476, where board.h has different LED/button enums.
if(NOT STEFI_BOARD STREQUAL "rohmi_g431")
    return()
endif()

add_executable(rohmi_boardtest boardtest.c)

target_link_libraries(rohmi_boardtest
        PRIVATE stefi::board stefi::simpleio stefi::oled stefi::motor stefi::line_sensor stefi::imu)
