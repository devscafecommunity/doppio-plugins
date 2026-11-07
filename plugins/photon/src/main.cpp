#include "AoBaker.hpp"
#include "caffeine/photon/Photon.hpp"
#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"

#include "ecs/ComponentQuery.hpp"
#include "ecs/MeshComponents.hpp"
#include "ecs/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <vector>

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

using Caffeine::ECS::PhotonGIComponent;

enum Preset : int {
    PresetOff = 0,
    PresetPerformance,
    PresetBalanced,
    PresetHigh,
    PresetCinematic,
    PresetCount
};

const char* const kPresetNames[PresetCount] = {
    "Off", "Performance", "Balanced", "High", "Cinematic"
};
const char* const kQualityNames[] = {"Low", "Medium", "High"};
const char* const kDebugNames[] = {"Lit scene", "GI only", "Fallback only"};

void applyPreset(int preset, PhotonGIComponent& gi) {
    switch (preset) {
        case PresetOff:
            gi.enabled = false;
            break;
        case PresetPerformance:
            gi.enabled = true;
            gi.quality = 1;
            gi.cascades = 3;
            gi.intensity = 0.8f;
            gi.radius = 10.0f;
            gi.thickness = 0.10f;
            gi.skyIntensity = 0.55f;
            gi.ambientIntensity = 0.18f;
            break;
        case PresetBalanced:
            gi.enabled = true;
            gi.quality = 2;
            gi.cascades = 4;
            gi.intensity = 1.1f;
            gi.radius = 14.0f;
            gi.thickness = 0.08f;
            gi.skyIntensity = 0.65f;
            gi.ambientIntensity = 0.20f;
            break;
        case PresetHigh:
            gi.enabled = true;
            gi.quality = 3;
            gi.cascades = 5;
            gi.intensity = 1.25f;
            gi.radius = 22.0f;
            gi.thickness = 0.06f;
            gi.skyIntensity = 0.70f;
            gi.ambientIntensity = 0.22f;
            break;
        case PresetCinematic:
            gi.enabled = true;
            gi.quality = 3;
            gi.cascades = 6;
            gi.intensity = 1.5f;
            gi.radius = 32.0f;
            gi.thickness = 0.05f;
            gi.skyIntensity = 0.80f;
            gi.ambientIntensity = 0.25f;
            break;
        default:
            break;
    }
    gi.tintR = 0.82f;
    gi.tintG = 0.82f;
    gi.tintB = 0.82f;
}

const char* classify(const PhotonGIComponent& gi) {
    if (!gi.enabled || gi.quality == 0) return "Off";
    if (gi.cascades >= 5) return "High quality";
    if (gi.cascades >= 4) return "Balanced";
    return "Performance";
}

Caffeine::ECS::Entity findPhotonEntity(Caffeine::ECS::World& world) {
    Caffeine::ECS::ComponentQuery query;
    query.with<PhotonGIComponent>();
    Caffeine::ECS::Entity found = Caffeine::ECS::Entity::INVALID;
    world.forEach<PhotonGIComponent>(
        query, [&](Caffeine::ECS::Entity entity, PhotonGIComponent&) {
            if (!found.isValid()) found = entity;
        });
    return found;
}

Caffeine::ECS::Entity ensureTarget(const Caffeine::Editor::PluginHostApi* host,
                                   Caffeine::ECS::World& world, bool create) {
    const Caffeine::ECS::Entity selected =
        Caffeine::Editor::PluginSelectedEntity(host, &world);
    if (selected.isValid() && world.has<PhotonGIComponent>(selected)) return selected;
    const Caffeine::ECS::Entity existing = findPhotonEntity(world);
    if (existing.isValid()) return existing;
    if (selected.isValid()) {
        auto& gi = world.add<PhotonGIComponent>(selected);
        applyPreset(PresetBalanced, gi);
        return selected;
    }
    if (!create) return Caffeine::ECS::Entity::INVALID;
    const Caffeine::ECS::Entity entity = world.create("Photon GI");
    auto& gi = world.add<PhotonGIComponent>(entity);
    applyPreset(PresetBalanced, gi);
    return entity;
}

class PhotonPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit PhotonPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;

        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.photon.panel";
        info.title = "Photon";
        info.defaultOpen = false;
        info.icon = "bluesky";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.24f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &PhotonPlugin::renderPanel, this);

        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                       "View/Photon Global Illumination",
                                       &PhotonPlugin::openPanel, this);
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                       "Plugins/Create Photon GI",
                                       &PhotonPlugin::createGI, this);
        }

        if (m_host->registerComponentDrawer && m_host->getComponentTypeId) {
            const Caffeine::u32 typeId =
                m_host->getComponentTypeId(m_host->editorContext, "PhotonGI");
            if (typeId != Caffeine::u32_max) {
                m_host->registerComponentDrawer(m_host->editorContext, GetName(), typeId,
                                                &PhotonPlugin::drawComponent, this);
            }
        }
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Photon"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Photon: hybrid radiance-cascade global illumination (screen-space near field + sky fallback).";
    }

private:
    static void openPanel(void* userData) {
        auto* self = static_cast<PhotonPlugin*>(userData);
        if (self->m_host && self->m_host->openPanel) {
            self->m_host->openPanel(self->m_host->editorContext, "Photon");
        }
    }

    static void markDirty(PhotonPlugin* self) {
        Caffeine::Editor::PluginMarkDirty(self->m_host);
    }

    static void createGI(void* userData) {
        auto* self = static_cast<PhotonPlugin*>(userData);
        Caffeine::ECS::World* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) return;
        const Caffeine::ECS::Entity entity = ensureTarget(self->m_host, *world, true);
        if (entity.isValid()) {
            if (auto* gi = world->get<PhotonGIComponent>(entity)) {
                applyPreset(PresetBalanced, *gi);
                markDirty(self);
            }
        }
        openPanel(self);
    }

    static void bakeSelectedAO(PhotonPlugin* self, Caffeine::ECS::World& world) {
        const Caffeine::ECS::Entity selected =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, &world);
        if (!selected.isValid()) {
            self->m_aoStatus = "Select a mesh entity to bake.";
            return;
        }
        Caffeine::PhotonAOBake::Settings settings;
        settings.resolution = self->m_aoResolution == 0 ? 256u
                              : self->m_aoResolution == 2 ? 1024u
                                                           : 512u;
        settings.rays = static_cast<Caffeine::u32>(
            std::clamp(static_cast<int>(self->m_aoRays), 1, 64));
        settings.maxDistance = std::max(self->m_aoMaxDistance, 0.01f);
        settings.bias = std::max(self->m_aoBias, 0.0f);
        const std::filesystem::path projectRoot =
            Caffeine::Editor::PluginProjectRoot(self->m_host);
        if (self->m_host && self->m_host->logInfo) {
            self->m_host->logInfo(self->m_host->editorContext,
                                  "Photon: baking ambient occlusion...");
        }
        const auto result = Caffeine::PhotonAOBake::bake(
            world, selected, settings, projectRoot.string());
        if (result.ok) {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer),
                          "Baked AO: %u/%u texels, %u triangles. ORM: %s", result.coveredTexels,
                          settings.resolution * settings.resolution, result.triangles,
                          result.ormPath.c_str());
            self->m_aoStatus = buffer;
            markDirty(self);
            Caffeine::Editor::PluginMarkAssetBrowserDirty(self->m_host);
        } else {
            self->m_aoStatus = "Bake failed: " + result.error;
            if (self->m_host && self->m_host->logError) {
                self->m_host->logError(self->m_host->editorContext,
                                       self->m_aoStatus.c_str());
            }
        }
    }

    static void bakeSceneAO(PhotonPlugin* self, Caffeine::ECS::World& world) {
        Caffeine::PhotonAOBake::Settings settings;
        settings.resolution = self->m_aoResolution == 0 ? 256u
                              : self->m_aoResolution == 2 ? 1024u
                                                           : 512u;
        settings.rays = static_cast<Caffeine::u32>(
            std::clamp(static_cast<int>(self->m_aoRays), 1, 64));
        settings.maxDistance = std::max(self->m_aoMaxDistance, 0.01f);
        settings.bias = std::max(self->m_aoBias, 0.0f);
        const std::filesystem::path projectRoot =
            Caffeine::Editor::PluginProjectRoot(self->m_host);

        std::vector<Caffeine::ECS::Entity> targets;
        Caffeine::ECS::ComponentQuery query;
        query.with<Caffeine::ECS::MeshFilterComponent>();
        world.forEach<Caffeine::ECS::MeshFilterComponent>(
            query, [&](Caffeine::ECS::Entity entity, Caffeine::ECS::MeshFilterComponent&) {
                targets.push_back(entity);
            });

        Caffeine::u32 baked = 0;
        Caffeine::u32 failed = 0;
        for (Caffeine::ECS::Entity entity : targets) {
            const auto result = Caffeine::PhotonAOBake::bake(
                world, entity, settings, projectRoot.string());
            if (result.ok) ++baked;
            else ++failed;
        }
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "Baked %u mesh(es), %u failed.", baked, failed);
        self->m_aoStatus = buffer;
        markDirty(self);
        Caffeine::Editor::PluginMarkAssetBrowserDirty(self->m_host);
    }

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<PhotonPlugin*>(userData);
        using namespace Caffeine::UIKit;

        Caffeine::ECS::World* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        PanelHeader("Photon", "Global illumination v0.1", "bluesky");

        if (!world) {
            EmptyState("No active scene", "Open or create a scene to enable Photon.", "bluesky");
            return;
        }

        Caffeine::ECS::Entity target = ensureTarget(self->m_host, *world, false);
        PhotonGIComponent* gi =
            target.isValid() ? world->get<PhotonGIComponent>(target) : nullptr;

        if (!gi) {
            EmptyState("Photon is not enabled",
                       "Create the GI component on the selected entity or a dedicated entity.",
                       "bluesky");
            if (PrimaryButton("Create Photon GI")) {
                createGI(self);
            }
            return;
        }

        if (Toggle("Enabled", &gi->enabled)) markDirty(self);
        SameLine();
        TextMuted(classify(*gi));

        if (Section("Presets", true)) {
            if (BeginPropertyGrid("photon_presets")) {
                if (ComboProperty("Quality preset", &self->m_preset, kPresetNames,
                                  PresetCount)) {
                    applyPreset(self->m_preset, *gi);
                    markDirty(self);
                }
                EndPropertyGrid();
            }
            if (ToolbarButton("Apply", "Apply the selected preset")) {
                applyPreset(self->m_preset, *gi);
                markDirty(self);
            }
            SameLine();
            if (ToolbarButton("Reset", "Reset to Balanced")) {
                self->m_preset = PresetBalanced;
                applyPreset(self->m_preset, *gi);
                markDirty(self);
            }
        }

        if (Section("Radiance cascades")) {
            bool changed = false;
            if (BeginPropertyGrid("photon_cascades")) {
                int quality = static_cast<int>(gi->quality);
                if (ComboProperty("Quality", &quality, kQualityNames, 3)) {
                    gi->quality = static_cast<Caffeine::u8>(std::clamp(quality + 1, 1, 3));
                    changed = true;
                }
                int cascades = static_cast<int>(gi->cascades);
                if (SliderIntProperty("Cascades", &cascades, 1, 6)) {
                    gi->cascades = static_cast<Caffeine::u8>(cascades);
                    changed = true;
                }
                changed |= SliderFloatProperty("Intensity", &gi->intensity, 0.0f, 8.0f);
                changed |= DragFloatProperty("Radius", &gi->radius, 0.1f, 0.1f, 500.0f);
                changed |= SliderFloatProperty("Thickness", &gi->thickness, 0.0f, 0.5f);
                EndPropertyGrid();
            }
            if (changed) markDirty(self);
        }

        if (Section("Far field fallback")) {
            bool changed = false;
            if (BeginPropertyGrid("photon_far")) {
                changed |= SliderFloatProperty("Sky intensity", &gi->skyIntensity, 0.0f, 4.0f);
                changed |= SliderFloatProperty("Ambient", &gi->ambientIntensity, 0.0f, 2.0f);
                changed |= SliderFloatProperty("Tint R", &gi->tintR, 0.0f, 2.0f);
                changed |= SliderFloatProperty("Tint G", &gi->tintG, 0.0f, 2.0f);
                changed |= SliderFloatProperty("Tint B", &gi->tintB, 0.0f, 2.0f);
                int debug = static_cast<int>(gi->debugView);
                if (ComboProperty("Debug", &debug, kDebugNames, 3)) {
                    gi->debugView = static_cast<Caffeine::u8>(debug);
                    changed = true;
                }
                EndPropertyGrid();
            }
            if (changed) markDirty(self);
        }

        if (Section("Baked ambient occlusion")) {
            TextMuted("Precomputed static AO written to the material ORM map. No RTX required.");
            if (BeginPropertyGrid("photon_ao")) {
                static const char* const kResolutions[] = {"256", "512", "1024"};
                ComboProperty("Resolution", &self->m_aoResolution, kResolutions, 3);
                SliderFloatProperty("Rays", &self->m_aoRays, 1.0f, 64.0f, "%.0f");
                DragFloatProperty("Max distance", &self->m_aoMaxDistance, 0.1f, 0.05f, 50.0f);
                DragFloatProperty("Bias", &self->m_aoBias, 0.001f, 0.0f, 0.2f);
                EndPropertyGrid();
            }
            if (ToolbarButton("Bake selected mesh", "Bake AO into the selected mesh material")) {
                bakeSelectedAO(self, *world);
            }
            SameLine();
            if (ToolbarButton("Bake scene", "Bake AO for every static mesh in the scene")) {
                bakeSceneAO(self, *world);
            }
            TextMuted("The bake blocks the editor briefly on large meshes.");
            if (!self->m_aoStatus.empty()) TextMuted(self->m_aoStatus.c_str());
        }

        SeparatorSpaced();
        TextMuted("Near field: screen-space cascades. Far field: sky/ambient fallback.");
        TextMutedF("Target entity: %u", target.id());
#endif
    }

    static void drawComponent(void* componentData, void* userData) {
#ifdef CF_HAS_IMGUI
        auto* gi = static_cast<PhotonGIComponent*>(componentData);
        auto* self = static_cast<PhotonPlugin*>(userData);
        if (!gi) return;
        using namespace Caffeine::UIKit;

        bool changed = false;
        if (BeginPropertyGrid("photon_component")) {
            int quality = static_cast<int>(gi->quality);
            if (ComboProperty("Quality", &quality, kQualityNames, 3)) {
                gi->quality = static_cast<Caffeine::u8>(std::clamp(quality + 1, 1, 3));
                changed = true;
            }
            int cascades = static_cast<int>(gi->cascades);
            if (SliderIntProperty("Cascades", &cascades, 1, 6)) {
                gi->cascades = static_cast<Caffeine::u8>(cascades);
                changed = true;
            }
            changed |= SliderFloatProperty("Intensity", &gi->intensity, 0.0f, 8.0f);
            changed |= DragFloatProperty("Radius", &gi->radius, 0.1f, 0.1f, 500.0f);
            changed |= SliderFloatProperty("Thickness", &gi->thickness, 0.0f, 0.5f);
            changed |= SliderFloatProperty("Sky intensity", &gi->skyIntensity, 0.0f, 4.0f);
            changed |= SliderFloatProperty("Ambient", &gi->ambientIntensity, 0.0f, 2.0f);
            EndPropertyGrid();
        }
        if (changed) markDirty(self);
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
    int m_preset = PresetBalanced;
    int m_aoResolution = 1;
    float m_aoRays = 16.0f;
    float m_aoMaxDistance = 2.5f;
    float m_aoBias = 0.02f;
    std::string m_aoStatus;
};

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    return new PhotonPlugin(host);
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
}
