#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "ProceduralBridge.hpp"

extern "C" void caffeine_procedural_plugin_register_services(
    const Caffeine::Editor::PluginHostApi* host);

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

#include <cstring>
#include <filesystem>
#include <string>

namespace {

#ifdef CF_HAS_IMGUI
static const char* kTerrainProfiles[] = {"flat", "hills", "fbm"};
static const char* kTerrainProfileLabels[] = {"Flat", "Hills (FBM)", "FBM (generic)"};
static const char* kStructureProfiles[] = {"none", "markers", "rooms", "track"};
static const char* kStructureProfileLabels[] = {"None", "Random markers", "Room grid",
                                                "Race track walls"};
static const char* kBenchmarks[] = {"exploration", "backrooms", "endless_race"};
static const char* kBenchmarkLabels[] = {"Exploration (hills + markers)",
                                         "Backrooms (flat + rooms)",
                                         "Endless race (flat + track)"};
#endif

class ProceduralToolsPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit ProceduralToolsPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        caffeine_procedural_plugin_register_services(m_host);
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.procedural.tools";
        info.title = "Procedural Tools";
        info.defaultOpen = false;
        info.icon = "terrain";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.23f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &ProceduralToolsPlugin::renderPanel, this);
        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(
                m_host->editorContext, GetName(), "Game/Install Procedural Scripts",
                &ProceduralToolsPlugin::installScriptsMenu, this);
        }
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Procedural Tools"; }
    const char* GetVersion() const override { return "0.2.0"; }
    const char* GetDescription() const override {
        return "Modular procedural streaming tools — terrain, structures, and Lua directors.";
    }

private:
    static void installScriptsMenu(void* userData) {
        auto* self = static_cast<ProceduralToolsPlugin*>(userData);
        const std::filesystem::path root = Caffeine::Editor::PluginProjectRoot(self->m_host);
        if (root.empty()) return;
        std::string error;
        if (!Caffeine::Editor::ProceduralBridge::installScriptTemplates(root, error)) {
            if (self->m_host->logError) self->m_host->logError(self->m_host->editorContext, error.c_str());
        }
    }

#ifdef CF_HAS_IMGUI
    bool invokeStreamingSetup(Caffeine::ECS::World* world) const {
        if (!world) return false;
        Caffeine::Editor::ProceduralStreamingSetup setup;
        setup.seed = static_cast<Caffeine::u32>(m_seed);
        setup.chunkSize = m_chunkSize;
        setup.viewRadius = static_cast<Caffeine::u32>(m_viewRadius);
        setup.scriptPath = m_scriptPath;
        setup.terrainProfile = kTerrainProfiles[m_terrainProfileIndex];
        setup.structureProfile = kStructureProfiles[m_structureProfileIndex];

        Caffeine::ECS::Entity director;
        Caffeine::ECS::Entity terrain;
        std::string error;
        if (!Caffeine::Editor::ProceduralBridge::createStreamingSetup(*world, setup,
                                                                      director, terrain, error)) {
            if (m_host->logError) m_host->logError(m_host->editorContext, error.c_str());
            return false;
        }
        Caffeine::Editor::PluginSelectEntity(m_host, director);
        Caffeine::Editor::PluginMarkDirty(m_host);
        return true;
    }

    bool invokeBenchmark(Caffeine::ECS::World* world, int index) const {
        if (!world || index < 0 || index >= 3) return false;
        Caffeine::ECS::Entity director;
        Caffeine::ECS::Entity terrain;
        std::string error;
        if (!Caffeine::Editor::ProceduralBridge::loadBenchmark(
                *world, kBenchmarks[index], static_cast<Caffeine::u32>(m_seed),
                m_chunkSize, static_cast<Caffeine::u32>(m_viewRadius), director, terrain, error)) {
            if (m_host->logError) m_host->logError(m_host->editorContext, error.c_str());
            return false;
        }
        Caffeine::Editor::PluginSelectEntity(m_host, director);
        Caffeine::Editor::PluginMarkDirty(m_host);
        return true;
    }
#endif

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<ProceduralToolsPlugin*>(userData);
        using namespace Caffeine::UIKit;
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            EmptyState("No active scene", "Open or create a scene to author procedural streaming.",
                       "terrain");
            return;
        }

        PanelHeader("Procedural Tools", "Streaming terrain, structures and Lua directors",
                    "terrain");
        Paragraph(
            "Modular tools for procedural streaming: combine terrain generators, structure "
            "spawners, and Lua directors to build any game. Press Play to stream chunks.");

        if (Section("Streaming")) {
            if (BeginPropertyGrid("procedural_streaming")) {
                DragIntProperty("Seed", &self->m_seed, 1.0f, 0, 999999);
                DragFloatProperty("Chunk size (m)", &self->m_chunkSize, 4.0f, 32.0f, 256.0f,
                                  "%.0f");
                DragIntProperty("View radius (chunks)", &self->m_viewRadius, 1.0f, 2, 12);
                EndPropertyGrid();
            }
        }

        if (Section("Built-in generators")) {
            if (BeginPropertyGrid("procedural_generators")) {
                ComboProperty("Terrain height", &self->m_terrainProfileIndex,
                              kTerrainProfileLabels, IM_ARRAYSIZE(kTerrainProfileLabels));
                HelpMarker("Height profile baked per chunk. Override in Lua with "
                           "generateTerrainChunk / fillTerrainNoise.");
                ComboProperty("Structure spawner", &self->m_structureProfileIndex,
                              kStructureProfileLabels, IM_ARRAYSIZE(kStructureProfileLabels));
                HelpMarker("Optional cubes spawned per chunk. Use none and spawn via Lua for full "
                           "control.");
                EndPropertyGrid();
            }
        }

        if (Section("Director script")) {
            if (BeginPropertyGrid("procedural_director")) {
                InputTextProperty("Script path", self->m_scriptPath, sizeof(self->m_scriptPath));
                EndPropertyGrid();
            }
            if (ToolbarButton("Install Lua scripts to project",
                              "Copies templates to scripts/procedural/")) {
                installScriptsMenu(self);
            }
        }

        if (Section("Scene setup")) {
            if (PrimaryButton("Create Streaming Setup", ImVec2(-1, 0))) {
                if (self->invokeStreamingSetup(world) && self->m_host->logInfo) {
                    self->m_host->logInfo(self->m_host->editorContext,
                                          "Streaming setup created. Press Play to stream.");
                }
            }
            HelpMarker("Creates ProceduralTerrain + ProceduralDirector with your settings.");
        }

        if (Section("Lua API reference", false)) {
            Bullet("caffeine.procedural.stream(director, terrain, cx, cz)");
            Bullet("caffeine.procedural.generateTerrainChunk(...)");
            Bullet("caffeine.procedural.spawnCube(...)");
            Bullet("caffeine.procedural.fbm2d / noise2d / hash");
            Bullet("caffeine.procedural.getFocusChunk / isChunkLoaded");
        }

        if (Section("Benchmarks (examples)", false)) {
            Paragraph(
                "Reference setups — not game types. Study or duplicate these scripts, then mix "
                "tools for your own design.");
            for (int i = 0; i < 3; ++i) {
                if (ToolbarButton(kBenchmarkLabels[i], "Load this benchmark scene")) {
                    if (self->invokeBenchmark(world, i) && self->m_host->logInfo) {
                        self->m_host->logInfo(self->m_host->editorContext,
                                              "Benchmark scene loaded. Press Play to test.");
                    }
                }
                if (i < 2) SameLine();
            }
            SeparatorSpaced();
            TextMuted("Scripts: exploration_director.lua, backrooms_director.lua, "
                      "endless_race_director.lua, custom_director.lua");
        }
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
#ifdef CF_HAS_IMGUI
    int m_seed = 42;
    float m_chunkSize = 64.0f;
    int m_viewRadius = 4;
    int m_terrainProfileIndex = 1;
    int m_structureProfileIndex = 0;
    char m_scriptPath[256] = "scripts/procedural/custom_director.lua";
#endif
};

}  // namespace

extern "C" {

CAFFEINE_PLUGIN_API Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    return new ProceduralToolsPlugin(host);
}

CAFFEINE_PLUGIN_API void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) { delete plugin; }

}  // extern "C"
