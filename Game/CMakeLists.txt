# Grab your game source files
file(GLOB_RECURSE GAME_SOURCES "src/*.cpp")

# Define the executable
add_executable(Game ${GAME_SOURCES})

# Include any game-specific headers
target_include_directories(Game PRIVATE include)

# Link the Engine library and the required SFML modules
target_link_libraries(Game PRIVATE Engine SFML::Graphics SFML::Window SFML::System SFML::Main TGUI::TGUI)

add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Engine>
    $<TARGET_FILE:sfml-system>
    $<TARGET_FILE:sfml-window>
    $<TARGET_FILE:sfml-graphics>
    $<TARGET_FILE:tgui>
    $<TARGET_FILE_DIR:Game>
)