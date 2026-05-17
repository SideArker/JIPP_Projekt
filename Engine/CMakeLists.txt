# Grab all source and header files in the current dir
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS "include/*.hpp")

add_library(Engine SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS} "src/main.cpp" "src/Unit.cpp" "src/MapRenderer.cpp")

target_compile_definitions(Engine PRIVATE ENGINE_BUILD_DLL)

# Make the Engine/include folder visible to anything that links to this library
target_include_directories(Engine PUBLIC include)

# Link SFML 3 targets to the Engine
target_link_libraries(Engine PRIVATE SFML::Graphics SFML::Window SFML::System)