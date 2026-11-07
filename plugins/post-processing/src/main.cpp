#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/postprocess/PostProcessEditorUI.hpp"
#include "caffeine/postprocess/PostProcessPresets.hpp"
#include "ecs/PostProcessComponents.hpp"
#include "ecs/CameraComponents.hpp"

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

class PostProcessingPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit PostProcessingPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.post_processing.panel";
        info.title = "Post Processing";
        info.defaultOpen = false;
        info.icon = "effects";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.23f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &PostProcessingPlugin::renderPanel, this);
        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                     "Effects/Add Post Process to Camera",
                                     &PostProcessingPlugin::addToSelectedCamera, this);
        }
        if (m_host->registerComponentDrawer && m_host->getComponentTypeId) {
            const Caffeine::u32 typeId =
                m_host->getComponentTypeId(m_host->editorContext, "PostProcess");
            if (typeId != Caffeine::u32_max) {
                m_host->registerComponentDrawer(m_host->editorContext, GetName(), typeId,
                                                &PostProcessingPlugin::drawComponent, this);
            }
        }
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Post Processing"; }
    const char* GetVersion() const override { return "0.2.0"; }
    const char* GetDescription() const override {
        return "Modular post-processing stack: per-effect modules, Lua control, optional benchmarks.";
    }

private:
    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<PostProcessingPlugin*>(userData);
        using namespace Caffeine::UIKit;
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            EmptyState("No active scene", "Open or create a scene with a camera to author effects.",
                       "effects");
            return;
        }
        const Caffeine::ECS::Entity e =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!e.isValid()) {
            EmptyState("Select a camera entity",
                       "Combine effect modules below — Cinematic / Horror / Arcade are optional "
                       "benchmark looks, not game types.",
                       "effects");
            return;
        }

        PanelHeader("Post Processing", "GPU effect stack", "effects");
        TextMutedF("Target: entity %u", e.id());

        if (!world->has<Caffeine::ECS::PostProcessComponent>(e)) {
            if (PrimaryButton("Add Post Process Stack")) {
                world->add<Caffeine::ECS::PostProcessComponent>(e);
                Caffeine::Editor::PluginMarkDirty(self->m_host);
            }
            return;
        }

        auto* fx = world->get<Caffeine::ECS::PostProcessComponent>(e);
        if (!fx) return;

        Paragraph(
            "The same stack the viewport and gameplay preview run on the GPU. Enable each module "
            "on its own; cinematic, horror and arcade are optional looks, not game types.");

        if (Section("Benchmark looks (optional)")) {
            if (Caffeine::PostProcess::EditorUI::drawBenchmarkPresets(*fx)) {
                Caffeine::Editor::PluginMarkDirty(self->m_host);
            }
        }

        SeparatorSpaced();
        if (Caffeine::PostProcess::EditorUI::drawEffectStack(*fx)) {
            Caffeine::Editor::PluginMarkDirty(self->m_host);
        }
#else
        (void)userData;
#endif
    }

    static void addToSelectedCamera(void* userData) {
        auto* self = static_cast<PostProcessingPlugin*>(userData);
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) return;
        const Caffeine::ECS::Entity e =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        if (!e.isValid()) return;
        if (!world->has<Caffeine::ECS::Camera2DComponent>(e) &&
            !world->has<Caffeine::ECS::Camera3DComponent>(e)) {
            if (self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext,
                                       "Select a Camera2D or Camera3D entity first.");
            }
            return;
        }
        if (!world->has<Caffeine::ECS::PostProcessComponent>(e)) {
            world->add<Caffeine::ECS::PostProcessComponent>(e);
            Caffeine::Editor::PluginMarkDirty(self->m_host);
        }
        if (self->m_host->logInfo) {
            self->m_host->logInfo(self->m_host->editorContext, "Post-process stack added to camera.");
        }
    }

    static void drawComponent(void* componentData, void* userData) {
#ifdef CF_HAS_IMGUI
        (void)userData;
        auto* fx = static_cast<Caffeine::ECS::PostProcessComponent*>(componentData);
        if (!fx) return;
        Caffeine::PostProcess::EditorUI::drawEffectStack(*fx);
#else
        (void)componentData;
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
};

PostProcessingPlugin* g_plugin = nullptr;

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(const Caffeine::Editor::PluginHostApi* host) {
    if (!g_plugin) g_plugin = new PostProcessingPlugin(host);
    return g_plugin;
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}
