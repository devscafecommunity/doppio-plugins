cmake_minimum_required(VERSION 3.20)
project(doppio-plugins
    VERSION 0.1.0
    DESCRIPTION "Official and community plugins for the Doppio editor (Caffeine Engine)"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT CAFFEINE_ROOT)
    message(FATAL_ERROR
        "CAFFEINE_ROOT is required. Example:\n"
        "  cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine")
endif()

option(DOPPIO_PLUGINS_BUILD_TEMPLATE
    "Build the hello-plugin template (minimal SDK example)"
    ON)

option(DOPPIO_PLUGINS_BUILD_OFFICIAL
    "Build official plugins from plugins/"
    ON)

if(DOPPIO_PLUGINS_BUILD_TEMPLATE)
    add_subdirectory(templates/hello-plugin)
endif()

if(DOPPIO_PLUGINS_BUILD_OFFICIAL)
    add_subdirectory(plugins/hello-plugin)
    add_subdirectory(plugins/terrain-generator)
    add_subdirectory(plugins/procedural-tools)
    add_subdirectory(plugins/post-processing)
    add_subdirectory(plugins/git)
endif()
