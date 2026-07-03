add_library(Motor STATIC motor.c)
add_library(stefi::motor ALIAS Motor)

target_include_directories(Motor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Motor PUBLIC board_iface libstefi)
