#include "caffeine/environment/VolumetricClouds.hpp"
#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"

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
                                       "View/Volumetric Clouds & Mist",
                                       &VolumetricCloudsPlugin::openPanel, this);
            m_host->registerMenuAction(m_host->editorContext, GetName(),
                                       "Plugins/Create Volumetric Clouds & Mist",
                                       &VolumetricCloudsPlugin::createAtmosphere, this);
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

    static void createAtmosphere(void* userData) {
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        Caffeine::ECS::World* world =
            Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) return;
        const Caffeine::ECS::Entity target =
            ensureTarget(self->m_host, *world, true);
        if (auto* clouds = target.isValid()
                               ? world->get<VolumetricCloudsComponent>(target)
                               : nullptr) {
            evaluate(self->m_weather, *clouds);
            markDirty(self);
        }
        openPanel(self);
    }

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        using namespace Caffeine::UIKit;
        using namespace Caffeine::UIKit::Elements;

        UIContext ui;
        Column root("vcm_root");

        auto& header = root.emplace<Card>("vcm_header");
        header.emplace<Label>("VC&M", TextTone::Accent).setHeading(true);
        header.emplace<Label>("Volumetric Clouds & Mist", TextTone::Muted);

        Caffeine::ECS::World* world =
            Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            root.emplace<Elements::EmptyState>(
                "No active scene",
                "Open or create a scene to author the atmosphere.", "bluesky");
            ui.draw(root);
            return;
        }

        Caffeine::ECS::Entity target = ensureTarget(self->m_host, *world, false);
        VolumetricCloudsComponent* clouds =
            target.isValid() ? world->get<VolumetricCloudsComponent>(target) : nullptr;

        if (!clouds) {
            root.emplace<Elements::EmptyState>(
                "No atmosphere target",
                "Select an entity and press Generate, or create a dedicated atmosphere.",
                "bluesky");
            auto& create = root.emplace<Button>("Create atmosphere", ButtonVariant::Primary);
            create.setOnClick([self, world] {
                const Caffeine::ECS::Entity created =
                    ensureTarget(self->m_host, *world, true);
                if (auto* component = created.isValid()
                                          ? world->get<VolumetricCloudsComponent>(created)
                                          : nullptr) {
                    evaluate(self->m_weather, *component);
                    markDirty(self);
                }
            });
            ui.draw(root);
            return;
        }

        auto& state = root.emplace<Card>("vcm_state");
        auto& stateRow = state.emplace<Row>("vcm_state_row");
        stateRow.emplace<Checkbox>("Enabled", &clouds->enabled)
            .setOnChange([self](bool) { markDirty(self); });
        auto& classification = stateRow.emplace<Label>(classify(*clouds), TextTone::Muted);
        classification.setGrow(true);

        auto mark = [self](auto&&...) { markDirty(self); };

        auto& weather = root.emplace<Elements::Section>("Weather");
        auto& weatherGrid = weather.emplace<PropertyGrid>("vcm_weather");
        weatherGrid
            .typedRow<ChoiceProperty>(
                "Preset", self->m_preset,
                std::vector<std::string>{std::begin(kPresetNames), std::end(kPresetNames)})
            .setOnChange([self](int value) {
                applyPreset(value, self->m_weather);
            });
        weatherGrid.typedRow<FloatProperty>("Humidity", self->m_weather.humidity, 0.0f, 1.0f,
                                            0.01f, "%.2f", true);
        weatherGrid.typedRow<FloatProperty>("Instability", self->m_weather.instability, 0.0f,
                                            1.0f, 0.01f, "%.2f", true);
        weatherGrid.typedRow<FloatProperty>("Wind speed", self->m_weather.windSpeed, 0.0f,
                                            80.0f, 0.2f, "%.2f");
        weatherGrid.typedRow<FloatProperty>("Wind direction", self->m_weather.windDirection,
                                            0.0f, 360.0f, 1.0f, "%.1f");
        weatherGrid.typedRow<FloatProperty>("Seed", self->m_weather.seed, -10000.0f, 10000.0f,
                                            0.1f, "%.2f");
        weatherGrid
            .typedRow<ChoiceProperty>(
                "Quality", self->m_weather.quality,
                std::vector<std::string>{std::begin(kQualityNames), std::end(kQualityNames)})
            .setOnChange([self](int value) {
                self->m_weather.quality = std::clamp(value, 0, 2);
            });

        auto& weatherActions = weather.emplace<Row>("vcm_weather_actions");
        auto& generate = weatherActions.emplace<Button>("Generate", ButtonVariant::Primary);
        generate.setWidth(92.0f);
        generate.setOnClick([self, clouds] {
            evaluate(self->m_weather, *clouds);
            markDirty(self);
        });
        auto& randomize = weatherActions.emplace<Button>("Randomize");
        randomize.setWidth(96.0f);
        randomize.setTooltip("Randomize weather and seed");
        randomize.setOnClick([self, clouds] {
            const float t = std::fmod(self->m_weather.seed * 0.6180339f + 0.37f, 1.0f);
            self->m_weather.seed = std::fmod(self->m_weather.seed + 17.31f, 10000.0f);
            self->m_weather.humidity = std::clamp(0.25f + t * 0.65f, 0.0f, 1.0f);
            self->m_weather.instability =
                std::clamp(0.10f + std::fmod(t * 7.13f, 1.0f) * 0.8f, 0.0f, 1.0f);
            evaluate(self->m_weather, *clouds);
            markDirty(self);
        });
        auto& clear = weatherActions.emplace<Button>("Clear");
        clear.setWidth(64.0f);
        clear.setTooltip("Disable the cloud layer");
        clear.setOnClick([self, clouds] {
            clouds->enabled = false;
            markDirty(self);
        });

        auto& layer = root.emplace<Elements::Section>("Cloud layer");
        self->m_componentQuality = static_cast<int>(clouds->quality);
        auto& layerGrid = layer.emplace<PropertyGrid>("vcm_clouds");
        layerGrid.typedRow<IntProperty>("Quality", self->m_componentQuality, 1, 3, 1.0f, "%d",
                                        true)
            .setOnChange([self, clouds](int value) {
                clouds->quality = static_cast<Caffeine::u8>(value);
                markDirty(self);
            });
        layerGrid.typedRow<FloatProperty>("Coverage", clouds->coverage, 0.0f, 1.0f, 0.01f,
                                          "%.2f", true).setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Density", clouds->density, 0.0f, 8.0f, 0.01f,
                                          "%.2f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Base altitude", clouds->baseAltitude, 0.0f,
                                          20000.0f, 10.0f, "%.0f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Top altitude", clouds->topAltitude, 1.0f, 30000.0f,
                                          10.0f, "%.0f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Detail", clouds->detail, 0.0f, 1.0f, 0.01f, "%.2f",
                                          true).setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Wind speed", clouds->windSpeed, 0.0f, 120.0f, 0.2f,
                                          "%.2f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Wind direction", clouds->windDirection, 0.0f, 360.0f,
                                          1.0f, "%.1f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Seed", clouds->seed, -10000.0f, 10000.0f, 0.1f,
                                          "%.2f").setOnChange(mark);
        layerGrid.typedRow<FloatProperty>("Sun scatter", clouds->sunScatter, 0.0f, 3.0f, 0.01f,
                                          "%.2f", true).setOnChange(mark);

        auto& mist = root.emplace<Elements::Section>("Mist");
        auto& mistGrid = mist.emplace<PropertyGrid>("vcm_mist");
        mistGrid.typedRow<FloatProperty>("Density", clouds->mistDensity, 0.0f, 1.0f, 0.01f,
                                         "%.2f", true).setOnChange(mark);
        mistGrid.typedRow<FloatProperty>("Height", clouds->mistHeight, 1.0f, 500.0f, 1.0f,
                                         "%.0f").setOnChange(mark);
        mistGrid.typedRow<FloatProperty>("Falloff", clouds->mistFalloff, 0.0f, 2.0f, 0.01f,
                                         "%.2f").setOnChange(mark);

        root.emplace<Elements::Separator>();
        char layerInfo[128];
        std::snprintf(layerInfo, sizeof(layerInfo), "Layer: %.0f m -> %.0f m",
                      clouds->baseAltitude, clouds->topAltitude);
        root.emplace<Label>(layerInfo, TextTone::Muted);
        if (target.isValid()) {
            root.emplace<Label>("Target entity: " + std::to_string(target.id()),
                                TextTone::Muted);
        }

        ui.draw(root);
#else
        (void)userData;
#endif
    }

    static void drawComponent(void* componentData, void* userData) {
#ifdef CF_HAS_IMGUI
        auto* clouds = static_cast<VolumetricCloudsComponent*>(componentData);
        auto* self = static_cast<VolumetricCloudsPlugin*>(userData);
        if (!clouds) return;

        using namespace Caffeine::UIKit;
        using namespace Caffeine::UIKit::Elements;

        UIContext ui;
        Column root("vcm_component_root");
        root.emplace<Label>(classify(*clouds), TextTone::Muted);

        self->m_componentQuality = static_cast<int>(clouds->quality);
        auto& grid = root.emplace<PropertyGrid>("vcm_component");
        auto mark = [self](auto&&...) { markDirty(self); };
        grid.typedRow<IntProperty>("Quality", self->m_componentQuality, 0, 3, 1.0f, "%d", true)
            .setOnChange([self, clouds](int value) {
                clouds->quality = static_cast<Caffeine::u8>(value);
                markDirty(self);
            });
        grid.typedRow<FloatProperty>("Coverage", clouds->coverage, 0.0f, 1.0f, 0.01f, "%.2f",
                                     true).setOnChange(mark);
        grid.typedRow<FloatProperty>("Density", clouds->density, 0.0f, 8.0f, 0.01f, "%.2f")
            .setOnChange(mark);
        grid.typedRow<FloatProperty>("Base altitude", clouds->baseAltitude, 0.0f, 20000.0f,
                                     10.0f, "%.0f").setOnChange(mark);
        grid.typedRow<FloatProperty>("Top altitude", clouds->topAltitude, 1.0f, 30000.0f,
                                     10.0f, "%.0f").setOnChange(mark);
        grid.typedRow<FloatProperty>("Detail", clouds->detail, 0.0f, 1.0f, 0.01f, "%.2f", true)
            .setOnChange(mark);
        grid.typedRow<FloatProperty>("Wind speed", clouds->windSpeed, 0.0f, 120.0f, 0.2f,
                                     "%.2f").setOnChange(mark);
        grid.typedRow<FloatProperty>("Wind direction", clouds->windDirection, 0.0f, 360.0f,
                                     1.0f, "%.1f").setOnChange(mark);
        grid.typedRow<FloatProperty>("Seed", clouds->seed, -10000.0f, 10000.0f, 0.1f, "%.2f")
            .setOnChange(mark);
        grid.typedRow<FloatProperty>("Mist density", clouds->mistDensity, 0.0f, 1.0f, 0.01f,
                                     "%.2f", true).setOnChange(mark);
        grid.typedRow<FloatProperty>("Mist height", clouds->mistHeight, 1.0f, 500.0f, 1.0f,
                                     "%.0f").setOnChange(mark);

        ui.draw(root);
#else
        (void)componentData;
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
    WeatherModel m_weather{};
    int m_preset = PresetCumulus;
    int m_componentQuality = 2;
};

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    return new VolumetricCloudsPlugin(host);
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
}
