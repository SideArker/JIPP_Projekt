cmake_minimum_required(VERSION 3.14)
project(JIPP_Projekt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS ON)

# Download SFML 3.1.0 automatically for portability
include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.1.0
)
FetchContent_MakeAvailable(SFML)

# Download TGUI 1.12.1 automatically
FetchContent_Declare(
    TGUI
    GIT_REPOSITORY https://github.com/texus/TGUI.git
    GIT_TAG        v1.12.1
)

set(TGUI_BACKEND SFML_GRAPHICS CACHE STRING "TGUI backend" FORCE)

FetchContent_MakeAvailable(TGUI)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Include your sub-projects
add_subdirectory(Engine)
add_subdirectory(Game)
add_subdirectory(MapEditor)