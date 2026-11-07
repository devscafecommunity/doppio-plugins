#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

class HelloPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit HelloPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.hello.panel";
        info.title = "Hello Plugin";
        info.defaultOpen = false;
        info.icon = "hello";
        info.dock = Caffeine::Editor::PluginDockArea::Bottom;
        info.size = 0.20f;
        Caffeine::Editor::RegisterPluginPanel(m_host, "Hello Plugin", info,
                                              &HelloPlugin::renderPanel, this);
        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, "Hello Plugin", "Help/Hello from Plugin",
                                       &HelloPlugin::menuAction, this);
        }
    }

    void OnUnload() override {}

    void OnUpdate(float) override {}

    const char* GetName() const override { return "Hello Plugin"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetDescription() const override {
        return "Example Caffeine editor plugin with a custom panel.";
    }

private:
    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<HelloPlugin*>(userData);
        using namespace Caffeine::UIKit;
        PanelHeader("Hello Plugin", "Plugin SDK sample", "hello");
        Paragraph("Ola do plugin Caffeine!");
        Badge(self->GetVersion(), tokens().accent);
        TextMuted("The panel is rendered with the public Caffeine UI kit.");
        SeparatorSpaced();
        if (PrimaryButton("Log to console")) {
            if (self->m_host && self->m_host->logInfo) {
                self->m_host->logInfo(self->m_host->editorContext, "Hello Plugin button clicked");
            }
        }
        if (ToolbarButton("Secondary action", "Uses the shared widget vocabulary")) {
            if (self->m_host && self->m_host->logInfo) {
                self->m_host->logInfo(self->m_host->editorContext, "Hello Plugin secondary action");
            }
        }
#else
        (void)userData;
#endif
    }

    static void menuAction(void* userData) {
        auto* self = static_cast<HelloPlugin*>(userData);
        if (self->m_host && self->m_host->logInfo) {
            self->m_host->logInfo(self->m_host->editorContext, "Hello Plugin menu action triggered");
        }
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
};

HelloPlugin* g_plugin = nullptr;

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(const Caffeine::Editor::PluginHostApi* host) {
    if (!g_plugin) {
        g_plugin = new HelloPlugin(host);
    }
    return g_plugin;
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}
