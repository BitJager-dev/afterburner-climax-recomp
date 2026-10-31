# after_burner_climax - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(after_burner_climax LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(AFTERBURNERCLIMAX_SOURCES
    src/main.cpp
    src/game_menu.cpp
    src/display_setup.cpp
    src/package_setup.cpp
    src/portable_data.cpp
)

if(WIN32)
    add_executable(after_burner_climax WIN32 ${AFTERBURNERCLIMAX_SOURCES})
else()
    add_executable(after_burner_climax ${AFTERBURNERCLIMAX_SOURCES})
endif()

rexglue_setup_target(after_burner_climax GPU_PLUGINS xenos)
