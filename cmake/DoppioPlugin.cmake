# DoppioPlugin.cmake — shared CMake helper for building Doppio editor plugins.
#
# Usage:
#   list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
#   include(DoppioPlugin)
#   doppio_add_plugin(my_plugin SOURCES src/main.cpp)

cmake_minimum_required(VERSION 3.20)

function(_doppio_plugin_require_caffeine_root)
    if(NOT CAFFEINE_ROOT)
        message(FATAL_ERROR
            "CAFFEINE_ROOT is not set. Point it to a Caffeine Engine checkout:\n"
            "  cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine")
    endif()
    if(NOT EXISTS "${CAFFEINE_ROOT}/include/caffeine/plugin/PluginAPI.hpp")
        message(FATAL_ERROR
            "CAFFEINE_ROOT does not contain the Plugin SDK header:\n"
            "  ${CAFFEINE_ROOT}/include/caffeine/plugin/PluginAPI.hpp")
    endif()
endfunction()

function(doppio_add_plugin target)
    set(options "")
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "doppio_add_plugin(${target}): SOURCES is required")
    endif()

    _doppio_plugin_require_caffeine_root()

    if(NOT IMGUI_DIR)
        if(EXISTS "${CAFFEINE_ROOT}/build/_deps/imgui-src")
            set(IMGUI_DIR "${CAFFEINE_ROOT}/build/_deps/imgui-src")
        elseif(DEFINED imgui_SOURCE_DIR)
            set(IMGUI_DIR "${imgui_SOURCE_DIR}")
        else()
            message(FATAL_ERROR
                "IMGUI_DIR is not set and could not be inferred from CAFFEINE_ROOT.\n"
                "Set -DIMGUI_DIR=/path/to/imgui (usually caffeine/build/_deps/imgui-src)")
        endif()
    endif()

    set(_output_name "${target}")
    if(ARG_OUTPUT_NAME)
        set(_output_name "${ARG_OUTPUT_NAME}")
    endif()

    add_library(${target} SHARED ${ARG_SOURCES})

    target_include_directories(${target} PRIVATE
        "${CAFFEINE_ROOT}/include"
        "${CAFFEINE_ROOT}/src"
        "${IMGUI_DIR}")

    target_compile_definitions(${target} PRIVATE
        CF_HAS_IMGUI=1
        CAFFEINE_PLUGIN_EXPORTS=1)

    target_compile_features(${target} PRIVATE cxx_std_20)

    set_target_properties(${target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${_output_name}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")

    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -Wno-macro-redefined -Wno-cpp)
    endif()
endfunction()
