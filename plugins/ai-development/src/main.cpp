#include "caffeine/navigation/NavigationApi.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"

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
        using namespace Caffeine::UIKit::Elements;

        UIContext ui;
        Column root("ai_development_root");

        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            root.emplace<Elements::EmptyState>(
                "No active scene", "Open or create a scene to author NPC behaviour.",
                "account");
            ui.draw(root);
            return;
        }
        const auto markDirty = [self] { Caffeine::Editor::PluginMarkDirty(self->m_host); };

        auto& header = root.emplace<Card>("ai_header");
        header.emplace<Label>("AI Development", TextTone::Accent).setHeading(true);
        header.emplace<Label>("NPC navigation and behaviour", TextTone::Muted);
        header.emplace<Elements::Paragraph>(
            "Program any NPC: a villager, a guide, a creature, or an opponent. The grid "
            "pathfinder lives in the engine; this panel only authors behaviour.");

        const Caffeine::ECS::Entity entity =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!entity.isValid()) {
            root.emplace<Elements::EmptyState>(
                "No entity selected",
                "Select an entity to add a nav volume or an agent.", "hierarchy");
            ui.draw(root);
            return;
        }
        header.emplace<Label>("Target: entity " + std::to_string(entity.id()),
                              TextTone::Muted);

        if (!world->has<Caffeine::Navigation::NavVolume>(entity)) {
            auto& addVolume = root.emplace<Button>("Add Nav Volume", ButtonVariant::Primary);
            addVolume.setOnClick([world, entity, markDirty] {
                Caffeine::Navigation::NavVolume volume;
                volume.resize(32, 32);
                world->add<Caffeine::Navigation::NavVolume>(entity, volume);
                markDirty();
            });
        } else if (auto* volume = world->get<Caffeine::Navigation::NavVolume>(entity)) {
            auto& navSection = root.emplace<Elements::Section>("Nav volume");
            self->m_navWidth = volume->width;
            self->m_navHeight = volume->height;
            auto& grid = navSection.emplace<PropertyGrid>("ai_nav_volume");
            grid.typedRow<IntProperty>("Width", self->m_navWidth, 2, 256, 1.0f, "%d")
                .setOnChange([self, volume, markDirty](int value) {
                    volume->resize(value, self->m_navHeight);
                    markDirty();
                });
            grid.typedRow<IntProperty>("Height", self->m_navHeight, 2, 256, 1.0f, "%d")
                .setOnChange([self, volume, markDirty](int value) {
                    volume->resize(self->m_navWidth, value);
                    markDirty();
                });
            grid.typedRow<FloatProperty>("Cell size", volume->cellSize, 0.1f, 100.0f, 0.05f,
                                         "%.2f")
                .setOnChange([markDirty](float) { markDirty(); });
            grid.typedRow<Vec3Property>("Origin", volume->origin, 0.1f)
                .setOnChange([markDirty] { markDirty(); });
        }

        if (!world->has<Caffeine::Navigation::NavAgent>(entity)) {
            auto& addAgent = root.emplace<Button>("Add Nav Agent", ButtonVariant::Primary);
            addAgent.setOnClick([world, entity, markDirty] {
                world->add<Caffeine::Navigation::NavAgent>(entity);
                markDirty();
            });
        } else if (auto* agent = world->get<Caffeine::Navigation::NavAgent>(entity)) {
            auto& agentSection = root.emplace<Elements::Section>("Agent");
            self->m_agentMode = static_cast<int>(agent->mode);
            auto& grid = agentSection.emplace<PropertyGrid>("ai_agent");
            grid.typedRow<ChoiceProperty>(
                "Behaviour", self->m_agentMode,
                std::vector<std::string>{"Idle", "Patrol", "Follow", "Scripted"})
                .setOnChange([agent, markDirty](int value) {
                    agent->mode = static_cast<Caffeine::Navigation::NavMode>(value);
                    agent->planDirty = true;
                    markDirty();
                });
            grid.typedRow<FloatProperty>("Speed", agent->speed, 0.0f, 100.0f, 0.05f, "%.2f")
                .setOnChange([markDirty](float) { markDirty(); });
            grid.typedRow<Vec3Property>("Destination", agent->destination, 0.1f)
                .setOnChange([agent, markDirty] {
                    agent->hasDestination = true;
                    agent->planDirty = true;
                    markDirty();
                });

            auto& patrolRow = agentSection.emplace<Row>("ai_patrol_actions");
            auto& addPatrol = patrolRow.emplace<Button>("Add patrol point from destination");
            addPatrol.setGrow(true);
            addPatrol.setOnClick([world, entity, agent, markDirty] {
                Caffeine::Navigation::addPatrolPoint(*world, entity, agent->destination);
                markDirty();
            });
            auto& clearPatrol = patrolRow.emplace<Button>("Clear");
            clearPatrol.setWidth(64.0f);
            clearPatrol.setOnClick([agent, markDirty] {
                agent->patrolCount = 0;
                agent->patrolIndex = 0;
                markDirty();
            });
            agentSection.emplace<Label>(
                "Patrol points: " + std::to_string(agent->patrolCount), TextTone::Muted);

            auto& scripts = agentSection.emplace<Elements::Section>("Behaviour scripts",
                                                                     false);
            const std::filesystem::path projectRoot =
                Caffeine::Editor::PluginProjectRoot(self->m_host);
            const std::filesystem::path scriptsRoot =
                projectRoot.empty() ? std::filesystem::path("scripts/ai")
                                    : projectRoot / "scripts" / "ai";
            auto save = [world, entity, self, scriptsRoot, markDirty](const char* file,
                                                                      const char* source) {
                const std::filesystem::path path = scriptsRoot / file;
                if (!writeTemplate(path, source)) return;
                Caffeine::Navigation::setBehaviorScript(*world, entity,
                                                        path.generic_string());
                Caffeine::Editor::PluginMarkAssetBrowserDirty(self->m_host);
                markDirty();
            };
            auto& scriptRow = scripts.emplace<Row>("ai_script_actions");
            scriptRow.emplace<Button>("Write idle.lua").setOnClick([save] {
                save("idle.lua", kIdleScript);
            });
            scriptRow.emplace<Button>("Write patrol.lua").setOnClick([save] {
                save("patrol.lua", kPatrolScript);
            });
            scriptRow.emplace<Button>("Write follow.lua").setOnClick([save] {
                save("follow.lua", kFollowScript);
            });
            if (agent->behaviorScript[0] != '\0') {
                scripts.emplace<Label>("Script: " + std::string(agent->behaviorScript),
                                       TextTone::Muted)
                    .setWrap(true);
            }
        }

        ui.draw(root);
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
    int m_navWidth = 32;
    int m_navHeight = 32;
    int m_agentMode = 0;
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
