# Board interface: header-only contract (config-struct types + extern table
# declarations in interface/stefi_board.h) that the component layer compiles
# against. Concrete boards implement it; the executable picks the
# implementation at link time.
add_library(board_iface INTERFACE)
add_library(stefi::board_iface ALIAS board_iface)
target_include_directories(board_iface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interface)
target_link_libraries(board_iface INTERFACE libstefi)

# Bundled concrete board implementation, selected by STEFI_BOARD. An external
# application can ignore this target and provide its own board.c (+ board.h
# with the device enumerators) against stefi::board_iface instead.
add_library(board STATIC ${STEFI_BOARD}/board.c)
add_library(stefi::board ALIAS board)

target_include_directories(board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${STEFI_BOARD})
target_link_libraries(board PUBLIC board_iface libstefi)
# board_init() brings up the SimpleIO devices and the OLED — a one-way
# dependency on the component layer (components link only board_iface, so
# the former board <-> SimpleIO cycle is gone).
target_link_libraries(board PRIVATE SimpleIO DisplayOLED)
