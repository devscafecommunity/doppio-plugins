include(DoppioPlugin)

doppio_add_plugin(terrain_plugin
    OUTPUT_NAME terrain_plugin
    SOURCES
        src/main.cpp
        src/TerrainBridge.cpp
        src/TerrainPluginServices.cpp
    PRIVATE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    LINK_LIBRARIES
        ${CMAKE_DL_LIBS})
