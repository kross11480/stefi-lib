add_library(LineSensor STATIC line_sensor.c)
add_library(stefi::line_sensor ALIAS LineSensor)

target_include_directories(LineSensor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(LineSensor PUBLIC board_iface libstefi)
