cmake_minimum_required(VERSION 3.20)
project(doppio-plugins
    VERSION 0.1.0
    DESCRIPTION "Official and community plugins for the Doppio editor (Caffeine Engine)"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(DOPPIO_PLUGINS_BUILD_TEMPLATE
    "Build the hello-plugin template (minimal SDK example)"
    ON)

option(DOPPIO_PLUGINS_BUILD_OFFICIAL
    "Build official plugins from the Caffeine monorepo (requires full engine checkout)"
    OFF)

if(DOPPIO_PLUGINS_BUILD_TEMPLATE)
    add_subdirectory(templates/hello-plugin)
endif()

if(DOPPIO_PLUGINS_BUILD_OFFICIAL)
    if(NOT CAFFEINE_ROOT)
        message(FATAL_ERROR
            "DOPPIO_PLUGINS_BUILD_OFFICIAL=ON requires -DCAFFEINE_ROOT=/path/to/caffeine")
    endif()
    message(STATUS "Official plugins are built from the Caffeine monorepo.")
    message(STATUS "  cmake --build ${CAFFEINE_ROOT}/build --target terrain_plugin git_plugin ...")
    message(STATUS "See plugins/README.md for the full target list.")
endif()
