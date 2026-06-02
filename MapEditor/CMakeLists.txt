file(GLOB_RECURSE MAPEDITOR_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(MapEditor ${MAPEDITOR_SOURCES})

target_include_directories(MapEditor PRIVATE include)

target_link_libraries(MapEditor PRIVATE Engine SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Main TGUI::TGUI)

add_custom_command(TARGET MapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-audio>
        $<TARGET_FILE:tgui>
        $<TARGET_FILE_DIR:MapEditor>

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Art"
        "$<TARGET_FILE_DIR:MapEditor>/Art"
)
