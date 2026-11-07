#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "ecs/TerrainComponents.hpp"
#include "TerrainBridge.hpp"
#include "TerrainEditorPanel.hpp"

extern "C" void caffeine_terrain_plugin_register_services(
    const Caffeine::Editor::PluginHostApi* host);

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

#include <cstring>

namespace {

class TerrainGeneratorPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit TerrainGeneratorPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        caffeine_terrain_plugin_register_services(m_host);
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.terrain.generator";
        info.title = "Terrain Generator";
        info.defaultOpen = false;
        info.icon = "terrain";
        info.dock = Caffeine::Editor::PluginDockArea::Bottom;
        info.size = 0.28f;
        Caffeine::Editor::RegisterPluginPanel(
            m_host, GetName(), info, &TerrainGeneratorPlugin::renderPanel, this);

        Caffeine::Editor::PluginPanelInfo editor;
        editor.id = "plugin.terrain.editor";
        editor.title = "Terrain Editor";
        editor.icon = "terrain";
        editor.dock = Caffeine::Editor::PluginDockArea::Bottom;
        editor.size = 0.28f;
        editor.defaultOpen = true;
        Caffeine::Editor::RegisterPluginPanel(
            m_host, GetName(), editor, &Caffeine::Plugins::Terrain::renderTerrainEditor,
            const_cast<Caffeine::Editor::PluginHostApi*>(m_host));
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Terrain Generator"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Ultra-realistic procedural terrain via domain warping + geomorphological PDE.";
    }

private:
    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<TerrainGeneratorPlugin*>(userData);
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            Caffeine::UIKit::EmptyState("No active scene", "Open or create a scene to generate terrain.",
                                        "terrain");
            return;
        }

        const Caffeine::ECS::Entity entity =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!entity.isValid() ||
            !world->has<Caffeine::ECS::TerrainComponent>(entity)) {
            Caffeine::UIKit::EmptyState("No terrain entity selected",
                                        "Select a Terrain entity, then choose a preset and click Generate.",
                                        "terrain");
            return;
        }

        auto* terrain = world->get<Caffeine::ECS::TerrainComponent>(entity);
        if (!terrain) return;

        using namespace Caffeine::UIKit;
        PanelHeader("Terrain Generator", "Domain warping + geomorphological PDE", "terrain");
        TextMutedF("Entity %u · %ux%u heightmap", entity.id(),
                   terrain->resolutionX, terrain->resolutionZ);

        static const char* kPresets[] = {"default", "alpine", "mesa", "volcanic", "glacial", "blend"};
        if (BeginPropertyGrid("terrain_generator_props")) {
            ComboProperty("Preset", &self->m_presetIndex, kPresets, IM_ARRAYSIZE(kPresets));
            DragFloatProperty("World size (m)", &self->m_worldSize, 4.0f, 32.0f, 4096.0f, "%.0f");
            DragIntProperty("Grid size", &self->m_gridSize, 8.0f, 64, 1024);
            DragIntProperty("Seed", &self->m_seed, 1.0f, 0, 999999);
            EndPropertyGrid();
        }

        SeparatorSpaced();
        if (self->m_busy) {
            TextMuted("Generating… (Node.js)");
        } else if (PrimaryButton("Generate Terrain")) {
            self->m_busy = true;
            std::string error;
            const bool ok = Caffeine::Editor::TerrainBridge::generateUltraRealistic(
                *world, entity, kPresets[self->m_presetIndex],
                static_cast<Caffeine::u32>(self->m_seed), self->m_worldSize,
                static_cast<Caffeine::u32>(self->m_gridSize), error);
            self->m_busy = false;
            if (ok) {
                Caffeine::Editor::PluginMarkDirty(self->m_host);
                if (self->m_host->logInfo) {
                    self->m_host->logInfo(self->m_host->editorContext, "Terrain generated.");
                }
            } else if (self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext, error.c_str());
            }
        }

        SeparatorSpaced();
        SectionLabel("Import");
        if (BeginPropertyGrid("terrain_import")) {
            InputTextProperty("Heightmap path", self->m_importPath, sizeof(self->m_importPath));
            EndPropertyGrid();
        }
        if (ToolbarButton("Import .txt heightmap")) {
            std::string error;
            if (Caffeine::Editor::TerrainBridge::importHeightmap(
                    *world, entity, self->m_importPath, terrain->worldSizeX,
                    terrain->worldSizeZ, error)) {
                Caffeine::Editor::PluginMarkDirty(self->m_host);
            } else if (self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext, error.c_str());
            }
        }

        TextMuted(
            "Algorithm: assets/general-ultra-realistic-terrain-algorithm (npm install on first run).");
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
#ifdef CF_HAS_IMGUI
    int m_presetIndex = 0;
    float m_worldSize = 256.0f;
    int m_gridSize = 257;
    int m_seed = 42;
    bool m_busy = false;
    char m_importPath[512] = {};
#endif
};

TerrainGeneratorPlugin* g_plugin = nullptr;

}  // namespace

extern "C" {

CAFFEINE_PLUGIN_API Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    g_plugin = new TerrainGeneratorPlugin(host);
    return g_plugin;
}

CAFFEINE_PLUGIN_API void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}

}  // extern "C"
