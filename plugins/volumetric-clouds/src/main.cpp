#include "caffeine/environment/VolumetricClouds.hpp"
#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"

#include "ecs/ComponentQuery.hpp"
#include "ecs/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

namespace {

using Caffeine::ECS::VolumetricCloudsComponent;

struct WeatherModel {
    float humidity = 0.55f;
    float instability = 0.38f;
    float windSpeed = 9.0f;
    float windDirection = 35.0f;
    float seed = 7.0f;
    int quality = 2;
};

enum Preset : int {
    PresetFair = 0,
    PresetCumulus,
    PresetOvercast,
    PresetStorm,
    PresetMistyMorning,
    PresetAlpine,
    PresetClear,
    PresetCount
};

const char* const kPresetNames[PresetCount] = {
    "Fair weather", "Cumulus", "Overcast", "Storm",
    "Misty morning", "Alpine", "Clear sky"
};

const char* const kQualityNames[] = {"Low", "Medium", "High"};

void applyPreset(int preset, WeatherModel& w) {
    switch (preset) {
        case PresetFair:
            w.humidity = 0.42f;
            w.instability = 0.28f;
            w.windSpeed = 7.0f;
            w.windDirection = 35.0f;
            w.quality = 2;
            break;
        case PresetCumulus:
            w.humidity = 0.56f;
            w.instability = 0.62f;
            w.windSpeed = 9.0f;
            w.windDirection = 50.0f;
            w.quality = 3;
            break;
        case PresetOvercast:
            w.humidity = 0.84f;
            w.instability = 0.30f;
            w.windSpeed = 12.0f;
            w.windDirection = 80.0f;
            w.quality = 2;
            break;
        case PresetStorm:
            w.humidity = 0.92f;
            w.instability = 0.88f;
            w.windSpeed = 28.0f;
            w.windDirection = 110.0f;
            w.quality = 3;
            break;
        case PresetMistyMorning:
            w.humidity = 0.72f;
            w.instability = 0.18f;
            w.windSpeed = 2.0f;
            w.windDirection = 20.0f;
            w.quality = 2;
            break;
        case PresetAlpine:
            w.humidity = 0.48f;
            w.instability = 0.70f;
            w.windSpeed = 16.0f;
            w.windDirection = 65.0f;
            w.quality = 3;
            break;
        case PresetClear:
        default:
            w.humidity = 0.20f;
            w.instability = 0.05f;
            w.windSpeed = 4.0f;
            w.windDirection = 0.0f;
            w.quality = 0;
            break;
    }
}

/// Weather model -> shader profile. This is the procedural core of VC&M:
/// humidity drives coverage and the layer thickness, instability adds detail
/// and vertical development, and calm humid air produces ground mist.
void evaluate(const WeatherModel& w, VolumetricCloudsComponent& c) {
    c.enabled = w.quality != 0 && w.humidity > 0.12f;
    c.quality = static_cast<Caffeine::u8>(std::clamp(w.quality + 1, 1, 3));
    c.coverage = std::clamp(0.08f + w.humidity * 0.80f + w.instability * 0.12f, 0.0f, 1.0f);
    c.density = 0.20f + w.humidity * 1.05f + w.instability * 0.85f;
    c.baseAltitude = 1650.0f - w.humidity * 720.0f;
    c.topAltitude = c.baseAltitude + 620.0f + w.humidity * 1350.0f +
                    w.instability * 700.0f;
    c.detail = 0.22f + w.instability * 0.70f;
    c.seed = w.seed;
    c.windSpeed = w.windSpeed;
    c.windDirection = w.windDirection;
    c.mistDensity = std::clamp((w.humidity - 0.46f) * 1.35f, 0.0f, 0.68f);
    c.mistHeight = 18.0f + w.humidity * 90.0f;
    c.mistFalloff = 0.35f;
    c.sunScatter = 1.0f;
    c.castShadows = 1;
}

const char* classify(const VolumetricCloudsComponent& c) {
    if (!c.enabled || c.quality == 0) return "Clear";
    if (c.mistDensity > 0.35f && c.coverage < 0.35f) return "Mist / valley fog";
    if (c.coverage > 0.82f) return "Overcast";
    if (c.density > 1.55f) return "Storm cells";
    if (c.detail > 0.62f) return "Cumulus";
    if (c.coverage > 0.55f) return "Broken cloud";
    return "Fair-weather cloud";
}

Caffeine::ECS::Entity findCloudEntity(Caffeine::ECS::World& world) {
    Caffeine::ECS::ComponentQuery query;
    query.with<VolumetricCloudsComponent>();
    Caffeine::ECS::Entity found = Caffeine::ECS::Entity::INVALID;
    world.forEach<VolumetricCloudsComponent>(
        query, [&](Caffeine::ECS::Entity entity, VolumetricCloudsComponent&) {
            if (!found.isValid()) found = entity;
        });
    return found;
}

Caffeine::ECS::Entity ensureTarget(const Caffeine::Editor::PluginHostApi* host,
                                   Caffeine::ECS::World& world, bool create) {
    const Caffeine::ECS::Entity selected =
        Caffeine::Editor::PluginSelectedEntity(host, &world);
    if (selected.isValid() && world.has<VolumetricCloudsComponent>(selected)) {
        return selected;
    }
    const Caffeine::ECS::Entity existing = findCloudEntity(world);
    if (existing.isValid()) return existing;
    if (selected.isValid()) {
        world.add<VolumetricCloudsComponent>(selected);
        return selected;
    }
    if (!create) return Caffeine::ECS::Entity::INVALID;
    const Caffeine::ECS::Entity entity = world.create("Volumetric Clouds");
    world.add<VolumetricCloudsComponent>(entity);
    return entity;
}

class VolumetricCloudsPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit VolumetricCloudsPlugin(const Caffeine::Editor::PluginHostApi* host)
        : m_host(host) {
        applyPreset(m_preset, m_weather);
    }

    void OnLoad() override {
        if (!m_host) return;

        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.volumetric_clouds.panel";
        info.title = "VC&M";
        info.defaultOpen = false;
        info.icon = "bluesky";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.24f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &VolumetricCloudsPlugin::renderPanel, this);

        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                       "Effects/Volumetric Clouds & Mist",
                                       &VolumetricCloudsPlugin::openPanel, this);
        }

        if (m_host->registerComponentDrawer && m_host->getComponentTypeId) {
            const Caffeine::u32 typeId =
                m_host->getComponentTypeId(m_host->editorContext, "VolumetricClouds");
            if (typeId != Caffeine::u32_max) {
                m_host->registerComponentDrawer(
                    m_host->editorContext, GetName(), typeId,
                    &VolumetricCloudsPlugin::drawComponent, this);
            }
        }
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "VC&M"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Volumetric Clouds & Mist: procedural sky clouds, storm cells and ground mist.";
    }

private:
    static void openPanel(void* userData) {
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        if (self->m_host && self->m_host->openPanel) {
            self->m_host->openPanel(self->m_host->editorContext, "VC&M");
        }
    }

    static void markDirty(VolumetricCloudsPlugin* self) {
        Caffeine::Editor::PluginMarkDirty(self->m_host);
    }

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        using namespace Caffeine::UIKit;

        Caffeine::ECS::World* world =
            Caffeine::Editor::PluginActiveWorld(self->m_host);
        PanelHeader("VC&M", "Volumetric Clouds & Mist", "bluesky");

        if (!world) {
            EmptyState("No active scene", "Open or create a scene to author the atmosphere.",
                       "bluesky");
            return;
        }

        Caffeine::ECS::Entity target = ensureTarget(self->m_host, *world, false);
        VolumetricCloudsComponent* clouds =
            target.isValid() ? world->get<VolumetricCloudsComponent>(target) : nullptr;

        if (!clouds) {
            EmptyState("No atmosphere target",
                       "Select an entity and press Generate, or create a dedicated atmosphere.",
                       "bluesky");
            if (PrimaryButton("Create atmosphere")) {
                target = ensureTarget(self->m_host, *world, true);
                clouds = target.isValid()
                             ? world->get<VolumetricCloudsComponent>(target)
                             : nullptr;
                if (clouds) {
                    evaluate(self->m_weather, *clouds);
                    markDirty(self);
                }
            }
            return;
        }

        if (Toggle("Enabled", &clouds->enabled)) markDirty(self);
        SameLine();
        TextMuted(classify(*clouds));

        if (Section("Weather", true)) {
            if (BeginPropertyGrid("vcm_weather")) {
                if (ComboProperty("Preset", &self->m_preset, kPresetNames, PresetCount)) {
                    applyPreset(self->m_preset, self->m_weather);
                }
                SliderFloatProperty("Humidity", &self->m_weather.humidity, 0.0f, 1.0f);
                SliderFloatProperty("Instability", &self->m_weather.instability, 0.0f, 1.0f);
                DragFloatProperty("Wind speed", &self->m_weather.windSpeed, 0.2f, 0.0f, 80.0f);
                DragFloatProperty("Wind direction", &self->m_weather.windDirection, 1.0f,
                                  0.0f, 360.0f);
                DragFloatProperty("Seed", &self->m_weather.seed, 0.1f, -10000.0f, 10000.0f);
                if (ComboProperty("Quality", &self->m_weather.quality, kQualityNames, 3)) {
                    self->m_weather.quality = std::clamp(self->m_weather.quality, 0, 2);
                }
                EndPropertyGrid();
            }

            if (PrimaryButton("Generate")) {
                evaluate(self->m_weather, *clouds);
                markDirty(self);
            }
            SameLine();
            if (ToolbarButton("Randomize", "Randomize weather and seed")) {
                const float t = std::fmod(self->m_weather.seed * 0.6180339f + 0.37f, 1.0f);
                self->m_weather.seed = std::fmod(self->m_weather.seed + 17.31f, 10000.0f);
                self->m_weather.humidity = std::clamp(0.25f + t * 0.65f, 0.0f, 1.0f);
                self->m_weather.instability = std::clamp(
                    0.10f + std::fmod(t * 7.13f, 1.0f) * 0.8f, 0.0f, 1.0f);
                evaluate(self->m_weather, *clouds);
                markDirty(self);
            }
            SameLine();
            if (ToolbarButton("Clear", "Disable the cloud layer")) {
                clouds->enabled = false;
                markDirty(self);
            }
        }

        if (Section("Cloud layer")) {
            bool changed = false;
            if (BeginPropertyGrid("vcm_clouds")) {
                int quality = static_cast<int>(clouds->quality);
                if (SliderIntProperty("Quality", &quality, 1, 3)) {
                    clouds->quality = static_cast<Caffeine::u8>(quality);
                    changed = true;
                }
                changed |= SliderFloatProperty("Coverage", &clouds->coverage, 0.0f, 1.0f);
                changed |= DragFloatProperty("Density", &clouds->density, 0.01f, 0.0f, 8.0f);
                changed |= DragFloatProperty("Base altitude", &clouds->baseAltitude, 10.0f,
                                             0.0f, 20000.0f);
                changed |= DragFloatProperty("Top altitude", &clouds->topAltitude, 10.0f,
                                             1.0f, 30000.0f);
                changed |= SliderFloatProperty("Detail", &clouds->detail, 0.0f, 1.0f);
                changed |= DragFloatProperty("Wind speed", &clouds->windSpeed, 0.2f, 0.0f,
                                             120.0f);
                changed |= DragFloatProperty("Wind direction", &clouds->windDirection, 1.0f,
                                             0.0f, 360.0f);
                changed |= DragFloatProperty("Seed", &clouds->seed, 0.1f, -10000.0f,
                                             10000.0f);
                changed |= SliderFloatProperty("Sun scatter", &clouds->sunScatter, 0.0f, 3.0f);
                EndPropertyGrid();
            }
            if (changed) markDirty(self);
        }

        if (Section("Mist")) {
            bool changed = false;
            if (BeginPropertyGrid("vcm_mist")) {
                changed |= SliderFloatProperty("Density", &clouds->mistDensity, 0.0f, 1.0f);
                changed |= DragFloatProperty("Height", &clouds->mistHeight, 1.0f, 1.0f, 500.0f);
                changed |= DragFloatProperty("Falloff", &clouds->mistFalloff, 0.01f, 0.0f, 2.0f);
                EndPropertyGrid();
            }
            if (changed) markDirty(self);
        }

        SeparatorSpaced();
        TextMutedF("Layer: %.0f m → %.0f m", clouds->baseAltitude, clouds->topAltitude);
        if (target.isValid()) {
            TextMutedF("Target entity: %u", target.id());
        }
#endif
    }

    static void drawComponent(void* componentData, void* userData) {
#ifdef CF_HAS_IMGUI
        auto* clouds = static_cast<VolumetricCloudsComponent*>(componentData);
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        if (!clouds) return;
        using namespace Caffeine::UIKit;

        bool changed = false;
        TextMuted(classify(*clouds));

        if (BeginPropertyGrid("vcm_component")) {
            int quality = static_cast<int>(clouds->quality);
            if (SliderIntProperty("Quality", &quality, 0, 3)) {
                clouds->quality = static_cast<Caffeine::u8>(quality);
                changed = true;
            }
            changed |= SliderFloatProperty("Coverage", &clouds->coverage, 0.0f, 1.0f);
            changed |= DragFloatProperty("Density", &clouds->density, 0.01f, 0.0f, 8.0f);
            changed |= DragFloatProperty("Base altitude", &clouds->baseAltitude, 10.0f,
                                         0.0f, 20000.0f);
            changed |= DragFloatProperty("Top altitude", &clouds->topAltitude, 10.0f,
                                         1.0f, 30000.0f);
            changed |= SliderFloatProperty("Detail", &clouds->detail, 0.0f, 1.0f);
            changed |= DragFloatProperty("Wind speed", &clouds->windSpeed, 0.2f, 0.0f, 120.0f);
            changed |= DragFloatProperty("Wind direction", &clouds->windDirection, 1.0f,
                                         0.0f, 360.0f);
            changed |= DragFloatProperty("Seed", &clouds->seed, 0.1f, -10000.0f, 10000.0f);
            changed |= SliderFloatProperty("Mist density", &clouds->mistDensity, 0.0f, 1.0f);
            changed |= DragFloatProperty("Mist height", &clouds->mistHeight, 1.0f, 1.0f, 500.0f);
            EndPropertyGrid();
        }
        if (changed) markDirty(self);
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
    WeatherModel m_weather{};
    int m_preset = PresetCumulus;
};

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    return new VolumetricCloudsPlugin(host);
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
}
