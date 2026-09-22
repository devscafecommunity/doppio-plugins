#include "caffeine/plugin/PluginAPI.hpp"

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

class HelloPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit HelloPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host || !m_host->registerPanel) {
            return;
        }
        m_host->registerPanel(m_host->editorContext, GetName(), "Hello Plugin",
                              &HelloPlugin::renderPanel, this);
        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                       "Help/Hello from Plugin", &HelloPlugin::menuAction, this);
        }
    }

    void OnUnload() override {}

    void OnUpdate(float) override {}

    const char* GetName() const override { return "Hello Plugin"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetDescription() const override {
        return "Minimal Doppio plugin — custom panel and menu action.";
    }

private:
    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<HelloPlugin*>(userData);
        ImGui::TextUnformatted("Hello from doppio-plugins!");
        ImGui::TextDisabled("Version %s", self->GetVersion());
        if (ImGui::Button("Log to console")) {
            if (self->m_host && self->m_host->logInfo) {
                self->m_host->logInfo(self->m_host->editorContext,
                                      "Hello Plugin button clicked");
            }
        }
#else
        (void)userData;
#endif
    }

    static void menuAction(void* userData) {
        auto* self = static_cast<HelloPlugin*>(userData);
        if (self->m_host && self->m_host->logInfo) {
            self->m_host->logInfo(self->m_host->editorContext,
                                  "Hello Plugin menu action triggered");
        }
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
};

HelloPlugin* g_plugin = nullptr;

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    if (!g_plugin) {
        g_plugin = new HelloPlugin(host);
    }
    return g_plugin;
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}
