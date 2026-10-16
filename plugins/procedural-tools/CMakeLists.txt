include(DoppioPlugin)

doppio_add_plugin(procedural_tools_plugin
    OUTPUT_NAME procedural_tools_plugin
    SOURCES
        src/main.cpp
        src/ProceduralBridge.cpp
        src/ProceduralPluginServices.cpp
    PRIVATE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    LINK_LIBRARIES
        ${CMAKE_DL_LIBS})
