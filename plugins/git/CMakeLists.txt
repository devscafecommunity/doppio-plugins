include(DoppioPlugin)

doppio_add_plugin(git_plugin
    OUTPUT_NAME git_plugin
    SOURCES
        src/main.cpp
        src/GitRunner.cpp
    PRIVATE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
