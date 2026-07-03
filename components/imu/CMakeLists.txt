add_library(Imu STATIC imu.c)
add_library(stefi::imu ALIAS Imu)

target_include_directories(Imu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Imu PUBLIC board_iface libstefi)
