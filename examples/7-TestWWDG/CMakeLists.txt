add_executable(test_wwdg test_wwdg.c)

target_link_libraries(test_wwdg PRIVATE stefi::board stefi::simpleio stefi::oled)
