add_executable(reaction_game reaction_game.c)

target_link_libraries(reaction_game PRIVATE stefi::board stefi::simpleio stefi::oled)
