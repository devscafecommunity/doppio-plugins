#include "TerrainEditorPanel.hpp"

#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/Widgets.hpp"

#include "assets/MeshCache.hpp"
#include "core/Types.hpp"
#include "ecs/Components.hpp"
#include "ecs/TerrainComponents.hpp"
#include "terrain/TerrainCache.hpp"
#include "terrain/TerrainGpuTextures.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>

namespace Caffeine::Plugins::Terrain {

namespace {

using Caffeine::ECS::Entity;
using Caffeine::ECS::World;
using Caffeine::Editor::PluginActiveWorld;
using Caffeine::Editor::PluginGetTerrainBrush;
using Caffeine::Editor::PluginHostApi;
using Caffeine::Editor::PluginMarkDirty;
using Caffeine::Editor::PluginProjectRoot;
using Caffeine::Editor::PluginSelectedEntity;
using Caffeine::Editor::PluginSetTerrainBrush;
using Caffeine::Editor::PluginTerrainBrush;
namespace UI = Caffeine::UIKit;

void copyCString(char* dest, Caffeine::usize destSize, const char* src) {
    if (!dest || destSize == 0) return;
    std::strncpy(dest, src ? src : "", destSize - 1);
    dest[destSize - 1] = '\0';
}

void drawEditTools(const PluginHostApi* host, World& world, Entity entity) {
    auto* terrain = world.get<Caffeine::ECS::TerrainComponent>(entity);
    if (!terrain) return;

    PluginTerrainBrush brush;
    if (!PluginGetTerrainBrush(host, brush)) return;

    const std::string root = PluginProjectRoot(host).string();

    if (UI::BeginPropertyGrid("terrain_editor_props")) {
        bool sculpt = brush.editMode == 1;
        bool splat = brush.editMode == 2;
        if (UI::CheckboxProperty("Sculpt mode", &sculpt)) {
            brush.editMode = sculpt ? 1 : (splat ? 2 : 0);
            if (sculpt) brush.gizmoMode = 0;
        }
        if (UI::CheckboxProperty("Splat paint", &splat)) {
            brush.editMode = splat ? 2 : (sculpt ? 1 : 0);
            if (splat) brush.gizmoMode = 0;
        }

        if (brush.editMode == 1) {
            static const char* kBrushLabels[] = {"Raise", "Lower", "Smooth", "Flatten", "Noise"};
            UI::ComboProperty("Brush", &brush.brushMode, kBrushLabels,
                              IM_ARRAYSIZE(kBrushLabels));
        } else if (brush.editMode == 2) {
            static const char* kLayerLabels[] = {"Grass", "Rock", "Sand", "Dirt"};
            int layer = static_cast<int>(brush.splatLayer);
            if (UI::ComboProperty("Paint layer", &layer, kLayerLabels,
                                  IM_ARRAYSIZE(kLayerLabels))) {
                brush.splatLayer = static_cast<unsigned int>(std::max(0, layer));
            }
        }

        if (brush.editMode != 0) {
            UI::SliderFloatProperty("Brush radius", &brush.radius, 0.5f, 64.0f, "%.1f");
            UI::SliderFloatProperty("Brush strength", &brush.strength, 0.01f, 1.0f, "%.2f");
            brush.radius = std::max(0.5f, brush.radius);
            brush.strength = std::max(0.01f, brush.strength);
        }
        UI::EndPropertyGrid();
    }
    PluginSetTerrainBrush(host, brush);

    UI::SeparatorSpaced();
    if (UI::ToolbarButton("Reset default textures",
                          "Restore the default terrain and splat textures")) {
        const Caffeine::ECS::TerrainComponent defaults;
        copyCString(terrain->texturePath, sizeof(terrain->texturePath), defaults.texturePath);
        for (Caffeine::u32 i = 0; i < Caffeine::ECS::kTerrainSplatLayerCount; ++i) {
            copyCString(terrain->splatLayerPaths[i], sizeof(terrain->splatLayerPaths[i]),
                        defaults.splatLayerPaths[i]);
        }
        Caffeine::Terrain::TerrainCache::instance().syncTextureToFilter(world, entity, *terrain);
#ifdef CF_HAS_SDL3
        Caffeine::Terrain::TerrainGpuTextureCache::instance().invalidateEntity(entity, nullptr);
#endif
        terrain->splatRevision++;
        PluginMarkDirty(host);
    }

    if (brush.editMode == 2) {
        UI::SeparatorSpaced();
        UI::SectionLabel("Layer textures");
        static const char* kLayerLabels[] = {"Grass", "Rock", "Sand", "Dirt"};
        for (Caffeine::u32 i = 0; i < Caffeine::ECS::kTerrainSplatLayerCount; ++i) {
            UI::PushId(static_cast<int>(i));
            const std::filesystem::path shown(terrain->splatLayerPaths[i]);
            UI::TextMutedF("%s: %s", kLayerLabels[i], shown.filename().string().c_str());
            if (UI::Hovered()) {
                UI::Tooltip(terrain->splatLayerPaths[i]);
            }
            if (terrain->splatLayerPaths[i][0] != '\0' &&
                Caffeine::Assets::MeshCache::resolveTexturePath(terrain->splatLayerPaths[i], root)
                    .empty()) {
                UI::TextDanger("missing");
            }
            UI::PopId();
        }
    }
}

void drawDataFile(const PluginHostApi* host, World& world, Entity entity) {
    auto* terrain = world.get<Caffeine::ECS::TerrainComponent>(entity);
    if (!terrain) return;

    if (UI::BeginPropertyGrid("terrain_editor_data")) {
        if (UI::InputTextProperty("Terrain path", terrain->terrainDataPath,
                                  sizeof(terrain->terrainDataPath))) {
            PluginMarkDirty(host);
        }
        UI::HelpMarker("Project-relative path to the .cterrain height/splat data file");
        UI::EndPropertyGrid();
    }

    if (UI::ToolbarButton("Export .cterrain", "Write the height and splat data to disk")) {
        const std::filesystem::path projectRoot = PluginProjectRoot(host);
        if (!projectRoot.empty()) {
            if (terrain->terrainDataPath[0] == '\0') {
                const std::string relPath =
                    "terrain/Terrain_" + std::to_string(entity.id()) + ".cterrain";
                copyCString(terrain->terrainDataPath, sizeof(terrain->terrainDataPath),
                            relPath.c_str());
            }
            Caffeine::Terrain::TerrainCache::instance().saveTerrainFile(
                entity, projectRoot / terrain->terrainDataPath, terrain->useSplatmap);
            PluginMarkDirty(host);
        }
    }
}

}  // namespace

void renderTerrainEditor(void* userData) {
    const auto* host = static_cast<const PluginHostApi*>(userData);
    if (!host) return;

    World* world = PluginActiveWorld(host);
    if (!world) {
        UI::EmptyState("No active scene", "Open or create a scene with terrain to sculpt or paint.",
                       "terrain");
        return;
    }

    const Entity entity = PluginSelectedEntity(host, world);
    if (!entity.isValid() ||
        !world->has<Caffeine::ECS::TerrainComponent>(entity)) {
        UI::EmptyState("No terrain entity selected",
                       "Select a Terrain entity, then use the Terrain Generator plugin to create "
                       "a heightmap.",
                       "terrain");
        return;
    }

    auto* terrain = world->get<Caffeine::ECS::TerrainComponent>(entity);
    if (!terrain) return;

    UI::PanelHeader("Terrain Editor", "Sculpt, paint and export terrain data", "terrain");
    if (UI::BeginTabs("##terrain_editor_tabs")) {
        if (UI::BeginTab("Sculpt / Paint")) {
            drawEditTools(host, *world, entity);
            UI::EndTab();
        }
        if (UI::BeginTab("Data")) {
            drawDataFile(host, *world, entity);
            UI::EndTab();
        }
        UI::EndTabs();
    }
}

}  // namespace Caffeine::Plugins::Terrain
