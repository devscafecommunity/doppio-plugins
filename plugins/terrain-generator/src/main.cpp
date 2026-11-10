#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"
#include "ecs/TerrainComponents.hpp"
#include "TerrainBridge.hpp"

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
        using namespace Caffeine::UIKit;
        using namespace Caffeine::UIKit::Elements;

        UIContext ui;
        Column root("terrain_generator_root");

        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            root.emplace<Elements::EmptyState>(
                "No active scene", "Open or create a scene to generate terrain.",
                "terrain");
            ui.draw(root);
            return;
        }

        const Caffeine::ECS::Entity entity =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!entity.isValid() ||
            !world->has<Caffeine::ECS::TerrainComponent>(entity)) {
            root.emplace<Elements::EmptyState>(
                "No terrain entity selected",
                "Select a Terrain entity, then choose a preset and click Generate.",
                "terrain");
            ui.draw(root);
            return;
        }

        auto* terrain = world->get<Caffeine::ECS::TerrainComponent>(entity);
        if (!terrain) return;

        static const char* kPresets[] = {"default", "arctic", "mesa", "volcanic",
                                         "glacial", "blend"};
        char info[128];
        std::snprintf(info, sizeof(info), "Entity %u - %ux%u heightmap", entity.id(),
                      terrain->resolutionX, terrain->resolutionZ);

        auto& card = root.emplace<Card>("terrain_generator_card");
        card.emplace<Label>("Terrain Generator", TextTone::Accent).setHeading(true);
        card.emplace<Label>("Domain warping + geomorphological PDE", TextTone::Muted);
        card.emplace<Label>(info, TextTone::Muted);

        auto& grid = card.emplace<PropertyGrid>("terrain_generator_props");
        grid.typedRow<ChoiceProperty>(
            "Preset", self->m_presetIndex,
            std::vector<std::string>{std::begin(kPresets), std::end(kPresets)});
        grid.typedRow<FloatProperty>("World size (m)", self->m_worldSize, 4.0f, 4096.0f,
                                     1.0f, "%.0f");
        grid.typedRow<IntProperty>("Grid size", self->m_gridSize, 64, 1024, 1.0f, "%d");
        grid.typedRow<IntProperty>("Seed", self->m_seed, 0, 999999, 1.0f, "%d");

        card.emplace<Elements::Separator>();
        if (self->m_busy) {
            card.emplace<Label>("Generating... (Node.js)", TextTone::Warning);
        }
        auto& generate = card.emplace<Button>("Generate Terrain", ButtonVariant::Primary);
        generate.setEnabled(!self->m_busy);
        generate.setOnClick([self, world, entity] {
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
                    self->m_host->logInfo(self->m_host->editorContext,
                                          "Terrain generated.");
                }
            } else if (self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext, error.c_str());
            }
        });

        auto& importSection = root.emplace<Elements::Section>("Import");
        auto& importGrid = importSection.emplace<PropertyGrid>("terrain_import");
        importGrid.typedRow<BufferProperty>("Heightmap path", self->m_importPath,
                                             sizeof(self->m_importPath));
        auto& importRow = importSection.emplace<Row>("terrain_import_row");
        auto& importButton = importRow.emplace<Button>("Import .txt heightmap");
        importButton.setGrow(true);
        importButton.setOnClick([self, world, entity, terrain] {
            std::string error;
            if (Caffeine::Editor::TerrainBridge::importHeightmap(
                    *world, entity, self->m_importPath, terrain->worldSizeX,
                    terrain->worldSizeZ, error)) {
                Caffeine::Editor::PluginMarkDirty(self->m_host);
            } else if (self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext, error.c_str());
            }
        });

        card.emplace<Label>(
            "Algorithm: assets/general-ultra-realistic-terrain-algorithm "
            "(npm install on first run).",
            TextTone::Muted)
            .setWrap(true);

        ui.draw(root);
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
