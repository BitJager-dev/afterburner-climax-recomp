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
    src/package_setup.cpp
)

if(WIN32)
    add_executable(after_burner_climax WIN32 ${AFTERBURNERCLIMAX_SOURCES})
else()
    add_executable(after_burner_climax ${AFTERBURNERCLIMAX_SOURCES})
endif()

rexglue_setup_target(after_burner_climax GPU_PLUGINS xenos)

# Linux: linker map for core-dump addresses, and load the SDK's shared
# libraries from next to the executable, so the staged dist/ folder runs from
# anywhere.
if(NOT WIN32)
    target_link_options(after_burner_climax PRIVATE "LINKER:-Map=${CMAKE_BINARY_DIR}/after_burner_climax.map")
    set_target_properties(after_burner_climax PROPERTIES BUILD_RPATH "$ORIGIN")
    add_custom_command(TARGET after_burner_climax POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:rex::runtime> $<TARGET_FILE_DIR:after_burner_climax>
        VERBATIM)
endif()
