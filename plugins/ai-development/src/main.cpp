#include "caffeine/navigation/NavigationApi.hpp"
#include "caffeine/plugin/PluginUI.hpp"

#include <filesystem>
#include <fstream>

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

const char* kIdleScript = R"(function onCreate(entity)
  caffeine.navigation.setMode(entity, "idle")
end

function onUpdate(entity, dt)
end
)";

const char* kPatrolScript = R"(function onCreate(entity)
  caffeine.navigation.setMode(entity, "patrol")
end

function onUpdate(entity, dt)
end
)";

const char* kFollowScript = R"(-- otherEntityId: id of the entity this NPC should follow.
function onCreate(entity)
  caffeine.navigation.setMode(entity, "follow")
end

function onUpdate(entity, dt)
end
)";

class AiDevelopmentPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit AiDevelopmentPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.ai.development";
        info.title = "AI Development";
        info.defaultOpen = false;
        info.icon = "account";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.23f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &AiDevelopmentPlugin::renderPanel, this);
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "AI Development"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Author NPC behaviour: idle, patrol, follow, or a Lua script. Not limited to combat.";
    }

private:
    static bool writeTemplate(const std::filesystem::path& path, const char* source) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path);
        if (!out) return false;
        out << source;
        return static_cast<bool>(out);
    }

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<AiDevelopmentPlugin*>(userData);
        using namespace Caffeine::UIKit;
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            EmptyState("No active scene", "Open or create a scene to author NPC behaviour.",
                       "account");
            return;
        }
        const auto markDirty = [self] { Caffeine::Editor::PluginMarkDirty(self->m_host); };
        PanelHeader("AI Development", "NPC navigation and behaviour", "account");
        Paragraph(
            "Program any NPC: a villager, a guide, a creature, or an opponent. The grid pathfinder "
            "lives in the engine; this panel only authors behaviour.");

        const Caffeine::ECS::Entity entity =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!entity.isValid()) {
            EmptyState("No entity selected", "Select an entity to add a nav volume or an agent.",
                       "hierarchy");
            return;
        }
        TextMutedF("Target: entity %u", entity.id());

        if (!world->has<Caffeine::Navigation::NavVolume>(entity)) {
            if (PrimaryButton("Add Nav Volume")) {
                Caffeine::Navigation::NavVolume volume;
                volume.resize(32, 32);
                world->add<Caffeine::Navigation::NavVolume>(entity, volume);
                markDirty();
            }
        } else if (auto* volume = world->get<Caffeine::Navigation::NavVolume>(entity)) {
            if (Section("Nav volume")) {
                if (BeginPropertyGrid("ai_nav_volume")) {
                    int size[2] = {volume->width, volume->height};
                    if (DragInt2Property("Cells", size, 1.0f, 2, 256)) {
                        volume->resize(size[0], size[1]);
                        markDirty();
                    }
                    if (DragFloatProperty("Cell size", &volume->cellSize, 0.05f, 0.1f, 100.0f)) {
                        markDirty();
                    }
                    if (PropertyRow("Origin", [&] {
                            return DragVec3("origin", &volume->origin.x, 0.1f);
                        })) {
                        markDirty();
                    }
                    EndPropertyGrid();
                }
            }
        }

        if (!world->has<Caffeine::Navigation::NavAgent>(entity)) {
            SeparatorSpaced();
            if (PrimaryButton("Add Nav Agent")) {
                world->add<Caffeine::Navigation::NavAgent>(entity);
                markDirty();
            }
        } else if (auto* agent = world->get<Caffeine::Navigation::NavAgent>(entity)) {
            if (Section("Agent")) {
                if (BeginPropertyGrid("ai_agent")) {
                    int mode = static_cast<int>(agent->mode);
                    const char* modes[] = {"Idle", "Patrol", "Follow", "Scripted"};
                    if (ComboProperty("Behaviour", &mode, modes, 4)) {
                        agent->mode = static_cast<Caffeine::Navigation::NavMode>(mode);
                        agent->planDirty = true;
                        markDirty();
                    }
                    if (DragFloatProperty("Speed", &agent->speed, 0.05f, 0.0f, 100.0f)) {
                        markDirty();
                    }
                    if (PropertyRow("Destination", [&] {
                            return DragVec3("destination", &agent->destination.x, 0.1f);
                        })) {
                        agent->hasDestination = true;
                        agent->planDirty = true;
                        markDirty();
                    }
                    EndPropertyGrid();
                }
                if (ToolbarButton("Add patrol point from destination")) {
                    Caffeine::Navigation::addPatrolPoint(*world, entity, agent->destination);
                    markDirty();
                }
                TextMutedF("Patrol points: %u", agent->patrolCount);
                if (ToolbarButton("Clear patrol")) {
                    agent->patrolCount = 0;
                    agent->patrolIndex = 0;
                    markDirty();
                }
            }

            if (Section("Behaviour scripts", false)) {
                const std::filesystem::path projectRoot =
                    Caffeine::Editor::PluginProjectRoot(self->m_host);
                const std::filesystem::path root = projectRoot.empty()
                                                       ? std::filesystem::path("scripts/ai")
                                                       : projectRoot / "scripts" / "ai";
                auto save = [&](const char* file, const char* source) {
                    const std::filesystem::path path = root / file;
                    if (!writeTemplate(path, source)) return;
                    Caffeine::Navigation::setBehaviorScript(*world, entity, path.generic_string());
                    Caffeine::Editor::PluginMarkAssetBrowserDirty(self->m_host);
                    markDirty();
                };
                if (ToolbarButton("Write idle.lua")) save("idle.lua", kIdleScript);
                SameLine();
                if (ToolbarButton("Write patrol.lua")) save("patrol.lua", kPatrolScript);
                SameLine();
                if (ToolbarButton("Write follow.lua")) save("follow.lua", kFollowScript);
                if (agent->behaviorScript[0] != '\0') {
                    TextMutedF("Script: %s", agent->behaviorScript);
                }
            }
        }
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
};

AiDevelopmentPlugin* g_plugin = nullptr;

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(const Caffeine::Editor::PluginHostApi* host) {
    if (!g_plugin) g_plugin = new AiDevelopmentPlugin(host);
    return g_plugin;
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}
