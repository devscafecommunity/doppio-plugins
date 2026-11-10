#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"
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
        using namespace Caffeine::UIKit::Elements;

        UIContext ui;
        Column root("procedural_tools_root");

        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            root.emplace<Elements::EmptyState>(
                "No active scene",
                "Open or create a scene to author procedural streaming.", "terrain");
            ui.draw(root);
            return;
        }

        auto& header = root.emplace<Card>("procedural_header");
        header.emplace<Label>("Procedural Tools", TextTone::Accent).setHeading(true);
        header.emplace<Label>("Streaming terrain, structures and Lua directors",
                              TextTone::Muted);
        header.emplace<Elements::Paragraph>(
            "Modular tools for procedural streaming: combine terrain generators, "
            "structure spawners, and Lua directors to build any game. Press Play to "
            "stream chunks.");

        auto& streaming = root.emplace<Elements::Section>("Streaming");
        auto& streamingGrid = streaming.emplace<PropertyGrid>("procedural_streaming");
        streamingGrid.typedRow<IntProperty>("Seed", self->m_seed, 0, 999999, 1.0f, "%d");
        streamingGrid.typedRow<FloatProperty>("Chunk size (m)", self->m_chunkSize, 32.0f,
                                              256.0f, 4.0f, "%.0f");
        streamingGrid.typedRow<IntProperty>("View radius (chunks)", self->m_viewRadius, 2, 12,
                                            1.0f, "%d");

        auto& generators = root.emplace<Elements::Section>("Built-in generators");
        auto& generatorGrid = generators.emplace<PropertyGrid>("procedural_generators");
        generatorGrid
            .typedRow<ChoiceProperty>(
                "Terrain height", self->m_terrainProfileIndex,
                std::vector<std::string>{std::begin(kTerrainProfileLabels),
                                         std::end(kTerrainProfileLabels)})
            .setHint("Height profile baked per chunk. Override in Lua with "
                     "generateTerrainChunk / fillTerrainNoise.");
        generatorGrid
            .typedRow<ChoiceProperty>(
                "Structure spawner", self->m_structureProfileIndex,
                std::vector<std::string>{std::begin(kStructureProfileLabels),
                                         std::end(kStructureProfileLabels)})
            .setHint("Optional cubes spawned per chunk. Use none and spawn via Lua for "
                     "full control.");

        auto& director = root.emplace<Elements::Section>("Director script");
        auto& directorGrid = director.emplace<PropertyGrid>("procedural_director");
        directorGrid.typedRow<BufferProperty>("Script path", self->m_scriptPath,
                                               sizeof(self->m_scriptPath));
        auto& installRow = director.emplace<Row>("procedural_install_row");
        auto& install = installRow.emplace<Button>("Install Lua scripts to project");
        install.setGrow(true);
        install.setTooltip("Copies templates to scripts/procedural/");
        install.setOnClick([self] { installScriptsMenu(self); });

        auto& setup = root.emplace<Elements::Section>("Scene setup");
        auto& setupRow = setup.emplace<Row>("procedural_setup_row");
        auto& create = setupRow.emplace<Button>("Create Streaming Setup",
                                                ButtonVariant::Primary);
        create.setGrow(true);
        create.setOnClick([self, world] {
            if (self->invokeStreamingSetup(world) && self->m_host->logInfo) {
                self->m_host->logInfo(self->m_host->editorContext,
                                      "Streaming setup created. Press Play to stream.");
            }
        });
        setup.emplace<Label>("Creates ProceduralTerrain + ProceduralDirector with your settings.",
                             TextTone::Muted)
            .setWrap(true);

        auto& api = root.emplace<Elements::Section>("Lua API reference", false);
        api.emplace<Label>("- caffeine.procedural.stream(director, terrain, cx, cz)",
                           TextTone::Muted);
        api.emplace<Label>("- caffeine.procedural.generateTerrainChunk(...)",
                           TextTone::Muted);
        api.emplace<Label>("- caffeine.procedural.spawnCube(...)", TextTone::Muted);
        api.emplace<Label>("- caffeine.procedural.fbm2d / noise2d / hash", TextTone::Muted);
        api.emplace<Label>("- caffeine.procedural.getFocusChunk / isChunkLoaded",
                           TextTone::Muted);

        auto& benchmarks = root.emplace<Elements::Section>("Benchmarks (examples)", false);
        benchmarks.emplace<Elements::Paragraph>(
            "Reference setups - not game types. Study or duplicate these scripts, then "
            "mix tools for your own design.");
        auto& benchmarkRow = benchmarks.emplace<Row>("procedural_benchmarks");
        for (int i = 0; i < 3; ++i) {
            auto& benchmark = benchmarkRow.emplace<Button>(kBenchmarkLabels[i]);
            benchmark.setGrow(true);
            benchmark.setTooltip("Load this benchmark scene");
            benchmark.setOnClick([self, world, i] {
                if (self->invokeBenchmark(world, i) && self->m_host->logInfo) {
                    self->m_host->logInfo(self->m_host->editorContext,
                                          "Benchmark scene loaded. Press Play to test.");
                }
            });
        }
        benchmarks.emplace<Elements::Separator>();
        benchmarks.emplace<Label>(
            "Scripts: exploration_director.lua, backrooms_director.lua, "
            "endless_race_director.lua, custom_director.lua",
            TextTone::Muted)
            .setWrap(true);

        ui.draw(root);
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
