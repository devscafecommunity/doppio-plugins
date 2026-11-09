include(DoppioPlugin)

# The simulation core is compiled into the plugin from the engine tree so the
# lab, the engine system and the plugin all share one source of truth.
doppio_add_plugin(natural_fluids_plugin
    OUTPUT_NAME natural_fluids_plugin
    SOURCES
        src/main.cpp
        ${CAFFEINE_ROOT}/src/caffeine/fluid/WaveField.cpp
        ${CAFFEINE_ROOT}/src/caffeine/fluid/ShallowWater.cpp
        ${CAFFEINE_ROOT}/src/caffeine/fluid/EulerFluid.cpp
        ${CAFFEINE_ROOT}/src/caffeine/fluid/FluidVolume.cpp
        ${CAFFEINE_ROOT}/src/caffeine/fluid/FluidWorld.cpp)
