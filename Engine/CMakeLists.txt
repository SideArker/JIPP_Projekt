# Grab all source and header files in the current dir 
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "include/*.hpp")

add_library(Engine SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_compile_definitions(Engine PRIVATE ENGINE_BUILD_DLL)

# Make the Engine/include folder visible to anything that links to this library
target_include_directories(Engine PUBLIC include)

# Link SFML 3 targets to the Engine
target_link_libraries(Engine PUBLIC SFML::Graphics SFML::Window SFML::System SFML::Audio TGUI::TGUI)
