#pragma once

#include "caffeine/plugin/PluginAPI.hpp"

namespace Caffeine::Plugins::Terrain {

/// Panel content for the terrain editor. The host wraps it with BeginPanel, so
/// this function only draws the body (tabs, brush controls and data export).
void renderTerrainEditor(void* userData);

}  // namespace Caffeine::Plugins::Terrain
