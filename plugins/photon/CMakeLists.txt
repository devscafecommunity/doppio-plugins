include(DoppioPlugin)

doppio_add_plugin(photon_plugin
    OUTPUT_NAME photon_plugin
    SOURCES
        src/main.cpp
        src/AoBaker.cpp)
