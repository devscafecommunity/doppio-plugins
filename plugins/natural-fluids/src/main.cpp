#include "caffeine/environment/NaturalFluids.hpp"
#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"

#include "ecs/Components.hpp"
#include "ecs/Components3D.hpp"
#include "ecs/MeshComponents.hpp"
#include "ecs/World.hpp"
#include "fluid/FluidWorld.hpp"
#include "scene/NaturalFluidSystem.hpp"

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

using Caffeine::ECS::NaturalFluidComponent;
using Caffeine::ECS::NaturalFluidMode;
using Caffeine::ECS::NaturalFluidPreset;

const char* const kModeNames[] = {
    "Flat water",
    "Wind + shallow physics",
    "Realistic water",
    "Fluid volume",
};

const char* const kPresetNames[] = {"Water", "Fire", "Fog", "Smoke"};

NaturalFluidComponent makeComponent(int mode) {
    NaturalFluidComponent component;
    component.mode = static_cast<Caffeine::u8>(std::clamp(mode, 0, 3));
    component.preset = static_cast<Caffeine::u8>(NaturalFluidPreset::Water);
    component.previewInEditor = 1;
    component.renderVolume = 1;
    component.windDirectionDeg = 35.0f;
    component.waterLevel = 0.0f;

    switch (static_cast<NaturalFluidMode>(component.mode)) {
        case NaturalFluidMode::FlatWater:
            component.windSpeed = 0.0f;
            component.shallowPhysics = 0;
            component.sizeX = 48.0f;
            component.sizeZ = 48.0f;
            component.surfaceResolution = 64;
            component.simulationResolution = 32;
            component.choppiness = 0.0f;
            component.roughness = 0.06f;
            break;
        case NaturalFluidMode::WindWater:
            component.windSpeed = 10.0f;
            component.shallowPhysics = 1;
            component.sizeX = 64.0f;
            component.sizeZ = 64.0f;
            component.surfaceResolution = 96;
            component.simulationResolution = 64;
            component.choppiness = 0.45f;
            component.amplitudeScale = 0.8f;
            break;
        case NaturalFluidMode::RealisticWater:
            component.windSpeed = 12.0f;
            component.shallowPhysics = 0;
            component.sizeX = 96.0f;
            component.sizeZ = 96.0f;
            component.surfaceResolution = 128;
            component.simulationResolution = 64;
            component.choppiness = 0.72f;
            component.amplitudeScale = 1.0f;
            break;
        case NaturalFluidMode::FluidVolume:
            component.shallowPhysics = 0;
            component.sizeX = 12.0f;
            component.sizeY = 12.0f;
            component.sizeZ = 12.0f;
            component.fluidResolution = 28;
            component.surfaceResolution = 64;
            component.simulationResolution = 64;
            component.fluidSourceRate = 0.45f;
            component.fluidSourceRadius = 2.0f;
            component.fluidSourceTemperature = 1.4f;
            component.fluidWindSpeed = 0.0f;
            break;
    }
    return component;
}

void applyPresetColors(NaturalFluidComponent& component, int preset) {
    component.preset = static_cast<Caffeine::u8>(std::clamp(preset, 0, 3));
    switch (static_cast<NaturalFluidPreset>(component.preset)) {
        case NaturalFluidPreset::Fire:
            component.fluidEmission = 2.5f;
            component.fluidSourceTemperature = 1.8f;
            component.fluidSourceRate = 0.55f;
            break;
        case NaturalFluidPreset::Fog:
            component.fluidEmission = 0.0f;
            component.fluidSourceTemperature = 0.2f;
            component.fluidSourceRate = 0.30f;
            break;
        case NaturalFluidPreset::Smoke:
            component.fluidEmission = 0.0f;
            component.fluidSourceTemperature = 1.1f;
            component.fluidSourceRate = 0.45f;
            break;
        case NaturalFluidPreset::Water:
        default:
            component.fluidEmission = 0.0f;
            component.fluidSourceTemperature = 0.0f;
            component.fluidSourceRate = 0.55f;
            break;
    }
}

void ensureRenderComponents(Caffeine::ECS::World& world, Caffeine::ECS::Entity entity,
                            const NaturalFluidComponent& component) {
    if (!world.has<Caffeine::ECS::Position3D>(entity) &&
        !world.has<Caffeine::ECS::Transform>(entity)) {
        world.add<Caffeine::ECS::Position3D>(entity);
    }
    if (!world.has<Caffeine::ECS::MeshFilterComponent>(entity)) {
        auto& filter = world.add<Caffeine::ECS::MeshFilterComponent>(entity);
        filter.primitive = Caffeine::ECS::MeshPrimitive::Custom;
    }
    if (!world.has<Caffeine::ECS::MeshRendererComponent>(entity)) {
        auto& renderer = world.add<Caffeine::ECS::MeshRendererComponent>(entity);
    }
    if (auto* renderer = world.get<Caffeine::ECS::MeshRendererComponent>(entity)) {
        const bool volume = component.mode ==
                            static_cast<Caffeine::u8>(NaturalFluidMode::FluidVolume);
        renderer->castShadows = component.castShadows != 0 && !volume;
        renderer->receiveShadows = component.receiveShadows != 0;
    }
}

Caffeine::ECS::Entity createFluidEntity(const Caffeine::Editor::PluginHostApi* host,
                                        Caffeine::ECS::World& world, int mode) {
    const NaturalFluidComponent component = makeComponent(mode);
    const bool volume = mode == static_cast<int>(NaturalFluidMode::FluidVolume);
    Caffeine::ECS::Entity entity = world.create(volume ? "Fluid Volume" : "Water Surface");
    auto& position = world.add<Caffeine::ECS::Position3D>(entity);
    position.position = Caffeine::Vec3(0.0f, volume ? component.sizeY * 0.5f : 0.0f, 0.0f);
    ensureRenderComponents(world, entity, component);
    world.add<NaturalFluidComponent>(entity, component);
    Caffeine::Editor::PluginSelectEntity(host, entity);
    Caffeine::Editor::PluginMarkDirty(host);
    return entity;
}

#ifdef CF_HAS_IMGUI
ImU32 previewColor(Caffeine::f32 density, Caffeine::f32 temperature, int preset) {
    const Caffeine::f32 d = std::clamp(density, 0.0f, 1.0f);
    const Caffeine::f32 t = std::clamp(temperature, 0.0f, 1.0f);
    Caffeine::f32 r = 0.0f;
    Caffeine::f32 g = 0.0f;
    Caffeine::f32 b = 0.0f;
    Caffeine::f32 a = 0.0f;
    switch (static_cast<NaturalFluidPreset>(preset)) {
        case NaturalFluidPreset::Fire:
            r = 0.25f + 0.75f * t + 0.2f * d;
            g = 0.10f + 0.65f * t;
            b = 0.02f + 0.10f * t;
            a = std::clamp(d + t * 0.9f, 0.0f, 1.0f);
            break;
        case NaturalFluidPreset::Fog:
            r = 0.78f;
            g = 0.80f;
            b = 0.84f;
            a = std::clamp(d * 1.1f, 0.0f, 1.0f);
            break;
        case NaturalFluidPreset::Smoke:
            r = 0.28f;
            g = 0.28f;
            b = 0.31f;
            a = std::clamp(d * 1.1f, 0.0f, 1.0f);
            break;
        case NaturalFluidPreset::Water:
        default:
            r = 0.05f;
            g = 0.30f;
            b = 0.70f;
            a = std::clamp(d * 1.2f, 0.0f, 1.0f);
            break;
    }
    return IM_COL32(static_cast<int>(r * 255.0f), static_cast<int>(g * 255.0f),
                    static_cast<int>(b * 255.0f), static_cast<int>(a * 255.0f));
}

void drawSurfacePreview(const Caffeine::Fluids::FluidWorld& fluid,
                        const NaturalFluidComponent& component, ImDrawList* drawList,
                        const ImVec2& origin, const ImVec2& size) {
    const int cells = 56;
    const Caffeine::f32 halfX = component.sizeX * 0.5f;
    const Caffeine::f32 halfZ = component.sizeZ * 0.5f;
    const Caffeine::f32 scale = std::max(fluid.waves().significantWaveHeight(), 0.25f);
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                            IM_COL32(8, 12, 20, 255));
    const ImVec2 cellSize(size.x / cells, size.y / cells);
    for (int j = 0; j < cells; ++j) {
        for (int i = 0; i < cells; ++i) {
            const Caffeine::f32 x = -halfX + (static_cast<Caffeine::f32>(i) + 0.5f) /
                                                 cells * component.sizeX;
            const Caffeine::f32 z = -halfZ + (static_cast<Caffeine::f32>(j) + 0.5f) /
                                                 cells * component.sizeZ;
            const Caffeine::Fluids::WaveSample sample = fluid.sampleSurface(x, z);
            const Caffeine::f32 height = std::clamp(sample.height / (scale * 2.0f) * 0.5f + 0.5f,
                                                    0.0f, 1.0f);
            const Caffeine::f32 foam = std::clamp(sample.foam, 0.0f, 1.0f);
            const int r = static_cast<int>((0.05f + height * 0.18f + foam * 0.72f) * 255.0f);
            const int g = static_cast<int>((0.20f + height * 0.40f + foam * 0.50f) * 255.0f);
            const int b = static_cast<int>((0.42f + height * 0.50f + foam * 0.20f) * 255.0f);
            const ImVec2 p0(origin.x + i * cellSize.x, origin.y + j * cellSize.y);
            const ImVec2 p1(p0.x + cellSize.x + 1.0f, p0.y + cellSize.y + 1.0f);
            drawList->AddRectFilled(p0, p1, IM_COL32(r, g, b, 255));
        }
    }
    drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                      IM_COL32(70, 82, 100, 255));
}

void draw2DPreview(const Caffeine::Fluids::EulerFluid2D& fluid,
                   const NaturalFluidComponent& component, ImDrawList* drawList,
                   const ImVec2& origin, const ImVec2& size) {
    const int n = fluid.resolution();
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                            IM_COL32(5, 6, 9, 255));
    const ImVec2 cellSize(size.x / n, size.y / n);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const Caffeine::f32 density = fluid.densityAt(i, j);
            const Caffeine::f32 temperature = fluid.temperatureAt(i, j);
            if (density < 0.004f && temperature < 0.02f) continue;
            const ImU32 color = previewColor(density, temperature, component.preset);
            const ImVec2 p0(origin.x + i * cellSize.x,
                            origin.y + (n - 1 - j) * cellSize.y);
            const ImVec2 p1(p0.x + cellSize.x + 1.0f, p0.y + cellSize.y + 1.0f);
            drawList->AddRectFilled(p0, p1, color);
        }
    }
    drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                      IM_COL32(70, 82, 100, 255));
}

void drawVolumePreview(const Caffeine::Fluids::FluidVolume& volume,
                       const NaturalFluidComponent& component, ImDrawList* drawList,
                       const ImVec2& origin, const ImVec2& size) {
    const int nx = volume.resolutionX();
    const int ny = volume.resolutionY();
    const int nz = volume.resolutionZ();
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, size.y + origin.y),
                            IM_COL32(5, 6, 9, 255));

    const int sliceY = ny / 2;
    const int sliceZ = nz / 2;
    const ImVec2 panelSize((size.x - 8.0f) * 0.5f, size.y);

    for (int pane = 0; pane < 2; ++pane) {
        const ImVec2 paneOrigin(origin.x + pane * (panelSize.x + 8.0f), origin.y);
        const int across = pane == 0 ? nx : nx;
        const int down = pane == 0 ? nz : ny;
        const ImVec2 cellSize(panelSize.x / across, panelSize.y / down);
        for (int v = 0; v < down; ++v) {
            for (int u = 0; u < across; ++u) {
                const Caffeine::f32 density =
                    pane == 0 ? volume.densityAt(u, sliceY, v) : volume.densityAt(u, v, sliceZ);
                const Caffeine::f32 temperature =
                    pane == 0 ? volume.temperatureAt(u, sliceY, v)
                              : volume.temperatureAt(u, v, sliceZ);
                if (density < 0.004f && temperature < 0.02f) continue;
                const ImU32 color = previewColor(density, temperature, component.preset);
                const ImVec2 p0(paneOrigin.x + u * cellSize.x,
                                paneOrigin.y + (down - 1 - v) * cellSize.y);
                const ImVec2 p1(p0.x + cellSize.x + 1.0f, p0.y + cellSize.y + 1.0f);
                drawList->AddRectFilled(p0, p1, color);
            }
        }
        drawList->AddRect(paneOrigin,
                          ImVec2(paneOrigin.x + panelSize.x, paneOrigin.y + panelSize.y),
                          IM_COL32(70, 82, 100, 255));
    }
}
#endif

class NaturalFluidsPlugin final : public Caffeine::Editor::IPlugin {
public:
    explicit NaturalFluidsPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.natural_fluids.panel";
        info.title = "Natural Fluids";
        info.defaultOpen = true;
        info.icon = "effects";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.30f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &NaturalFluidsPlugin::renderPanel, this);
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Natural Fluids"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Animated water surfaces and real Eulerian water, fire, fog and smoke.";
    }

private:
    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        using namespace Caffeine::UIKit;
        using namespace Caffeine::UIKit::UI;

        auto* self = static_cast<NaturalFluidsPlugin*>(userData);
        auto* world = Caffeine::Editor::PluginActiveWorld(self->m_host);
        if (!world) {
            Caffeine::UIKit::EmptyState("No active scene",
                       "Open or create a scene to add Natural Fluids.", "effects");
            return;
        }

        UIContext ctx;
        Column root("natural_fluids_root");

        Caffeine::ECS::Entity entity =
            Caffeine::Editor::PluginSelectedEntity(self->m_host, world);
        const bool hasComponent =
            entity.isValid() && world->has<NaturalFluidComponent>(entity);

        if (!hasComponent) {
            auto& card = root.emplace<Card>("create");
            card.emplace<Label>("Natural Fluids", TextTone::Accent).setHeading(true);
            card.emplace<UI::Paragraph>(
                "Select an entity with Natural Fluids, or create a water surface / fluid volume.");
            auto& actions = card.emplace<Row>("create_actions");
            actions.emplace<Button>("Realistic water", ButtonVariant::Primary)
                .setOnClick([&] {
                    entity = createFluidEntity(self->m_host, *world,
                                               static_cast<int>(NaturalFluidMode::RealisticWater));
                });
            actions.emplace<Button>("Flat water").setOnClick([&] {
                entity = createFluidEntity(self->m_host, *world,
                                           static_cast<int>(NaturalFluidMode::FlatWater));
            });
            actions.emplace<Button>("Wind + physics").setOnClick([&] {
                entity = createFluidEntity(self->m_host, *world,
                                           static_cast<int>(NaturalFluidMode::WindWater));
            });
            actions.emplace<Button>("Fluid volume").setOnClick([&] {
                entity = createFluidEntity(self->m_host, *world,
                                           static_cast<int>(NaturalFluidMode::FluidVolume));
            });
            if (entity.isValid()) {
                card.emplace<Button>("Add Natural Fluids to selected entity",
                                     ButtonVariant::Primary)
                    .setOnClick([&] {
                        NaturalFluidComponent component =
                            makeComponent(static_cast<int>(NaturalFluidMode::RealisticWater));
                        ensureRenderComponents(*world, entity, component);
                        world->add<NaturalFluidComponent>(entity, component);
                        Caffeine::Editor::PluginMarkDirty(self->m_host);
                    });
            }
            ctx.draw(root);
            return;
        }

        NaturalFluidComponent* component = world->get<NaturalFluidComponent>(entity);
        if (!component) return;

        const NaturalFluidComponent before = *component;
        const int mode = static_cast<int>(component->mode);
        const bool volumeMode = mode == static_cast<int>(NaturalFluidMode::FluidVolume);

        int level = mode;
        int preset = static_cast<int>(component->preset);
        int surfaceResolution = static_cast<int>(component->surfaceResolution);
        int simulationResolution = static_cast<int>(component->simulationResolution);
        int fluidResolution = static_cast<int>(component->fluidResolution);
        bool shallow = component->shallowPhysics != 0;
        bool preview = component->previewInEditor != 0;
        bool renderVolume = component->renderVolume != 0;
        bool simulateFluid2D = component->simulateFluid2D != 0;
        float color[4] = {component->albedoR, component->albedoG, component->albedoB,
                          component->albedoA};

        {
            auto& card = root.emplace<Card>("level");
            card.emplace<Label>("Simulation", TextTone::Accent).setHeading(true);
            auto& grid = card.emplace<PropertyGrid>("level_grid");
            grid.typedRow<ChoiceProperty>(
                "Level", level,
                std::vector<std::string>{kModeNames[0], kModeNames[1], kModeNames[2],
                                         kModeNames[3]});
            if (volumeMode) {
                grid.typedRow<ChoiceProperty>(
                    "Fluid", preset,
                    std::vector<std::string>{kPresetNames[0], kPresetNames[1],
                                             kPresetNames[2], kPresetNames[3]});
            }
        }

        {
            auto& card = root.emplace<Card>("properties");
            card.emplace<Label>("Properties", TextTone::Accent).setHeading(true);
            auto& grid = card.emplace<PropertyGrid>("properties_grid");
            if (!volumeMode) {
                grid.typedRow<FloatProperty>("Size X (m)", component->sizeX, 8.0f, 2048.0f,
                                             1.0f, "%.0f");
                grid.typedRow<FloatProperty>("Size Z (m)", component->sizeZ, 8.0f, 2048.0f,
                                             1.0f, "%.0f");
                grid.typedRow<FloatProperty>("Water level", component->waterLevel,
                                             -1000.0f, 1000.0f, 0.05f, "%.2f");
                grid.typedRow<FloatProperty>("Amplitude", component->amplitudeScale, 0.0f,
                                             3.0f, 0.01f, "%.2f", true);
                grid.typedRow<FloatProperty>("Choppiness", component->choppiness, 0.0f, 1.0f,
                                             0.01f, "%.2f", true);
                grid.typedRow<FloatProperty>("Wind speed (m/s)", component->windSpeed, 0.0f,
                                             60.0f, 0.2f, "%.1f");
                grid.typedRow<FloatProperty>("Wind direction",
                                             component->windDirectionDeg, 0.0f, 360.0f, 1.0f,
                                             "%.0f deg", true);
                grid.typedRow<FloatProperty>("Wavelength", component->wavelengthScale, 0.1f,
                                             4.0f, 0.01f, "%.2f", true);
                grid.typedRow<FloatProperty>("Foam threshold", component->foamThreshold, 0.0f,
                                             1.0f, 0.01f, "%.2f", true);
                grid.typedRow<BoolProperty>("Shallow physics", shallow);
                grid.typedRow<IntProperty>("Physics grid", simulationResolution, 16, 192,
                                           1.0f, "%d", true);
                grid.typedRow<IntProperty>("Mesh resolution", surfaceResolution, 16, 256,
                                           1.0f, "%d", true);
                grid.typedRow<BoolProperty>("2D fluid preview", simulateFluid2D);
            } else {
                grid.typedRow<FloatProperty>("Size X (m)", component->sizeX, 2.0f, 128.0f,
                                             0.5f, "%.1f");
                grid.typedRow<FloatProperty>("Size Y (m)", component->sizeY, 2.0f, 128.0f,
                                             0.5f, "%.1f");
                grid.typedRow<FloatProperty>("Size Z (m)", component->sizeZ, 2.0f, 128.0f,
                                             0.5f, "%.1f");
                grid.typedRow<IntProperty>("Volume grid", fluidResolution, 12, 48, 1.0f,
                                           "%d", true);
                grid.typedRow<FloatProperty>("Source rate", component->fluidSourceRate, 0.0f,
                                             4.0f, 0.02f, "%.2f");
                grid.typedRow<FloatProperty>("Source radius", component->fluidSourceRadius,
                                             0.2f, 12.0f, 0.1f, "%.2f");
                grid.typedRow<FloatProperty>("Source heat",
                                             component->fluidSourceTemperature, 0.0f, 6.0f,
                                             0.05f, "%.2f");
                grid.typedRow<FloatProperty>("Fluid wind", component->fluidWindSpeed, 0.0f,
                                             20.0f, 0.1f, "%.2f");
                grid.typedRow<FloatProperty>("Fluid wind dir",
                                             component->fluidWindDirectionDeg, 0.0f, 360.0f,
                                             1.0f, "%.0f deg", true);
                grid.typedRow<FloatProperty>("Emission", component->fluidEmission, 0.0f,
                                             20.0f, 0.1f, "%.2f");
                grid.typedRow<BoolProperty>("Render density voxels", renderVolume);
            }
            grid.typedRow<FloatProperty>("Time scale", component->timeScale, 0.0f, 4.0f,
                                         0.01f, "%.2f", true);
            grid.typedRow<BoolProperty>("Animate in editor", preview);
            grid.typedRow<ColorFieldProperty>("Surface colour", color);
            grid.typedRow<FloatProperty>("Roughness", component->roughness, 0.02f, 1.0f,
                                         0.01f, "%.2f", true);
        }

        auto* fluidWorld = Caffeine::Scene::naturalFluidWorldForEdit(*world, entity.id());
        {
            auto& card = root.emplace<Card>("preview");
            card.emplace<Label>("Preview", TextTone::Accent).setHeading(true);
            if (!fluidWorld) {
                card.emplace<Label>("Simulation starts on the next editor frame.",
                                    TextTone::Muted);
            } else {
                auto& actions = card.emplace<Row>("preview_actions");
                actions.emplace<Button>("Drop water", ButtonVariant::Toolbar)
                    .setOnClick([fluidWorld, component] {
                        fluidWorld->addWaterDrop(0.0f, 0.0f, 32.0f,
                                                 component->shallowDepth * 3.0f);
                    });
                actions.emplace<Button>("Impulse", ButtonVariant::Toolbar)
                    .setOnClick([fluidWorld] {
                        fluidWorld->addWaterImpulse(0.0f, 0.0f, 6.0f, 0.0f, 5.0f);
                    });
                actions.emplace<Button>("Puff", ButtonVariant::Toolbar)
                    .setOnClick([fluidWorld, component, volumeMode] {
                        if (volumeMode) {
                            fluidWorld->addVolumeSource(
                                component->sizeX * 0.5f, component->sizeY * 0.16f,
                                component->sizeZ * 0.5f, 1.0f,
                                component->fluidSourceRadius);
                            if (component->preset !=
                                static_cast<Caffeine::u8>(NaturalFluidPreset::Water)) {
                                fluidWorld->volume().addTemperature(
                                    component->sizeX * 0.5f, component->sizeY * 0.16f,
                                    component->sizeZ * 0.5f, 2.0f,
                                    component->fluidSourceRadius);
                            }
                        } else {
                            fluidWorld->addFluidSource(component->sizeX * 0.5f,
                                                       component->sizeZ * 0.18f, 1.0f, 3.0f);
                        }
                    });
                actions.emplace<Button>("Reset", ButtonVariant::Toolbar)
                    .setOnClick([world, entity] {
                        Caffeine::Scene::resetNaturalFluid(*world, entity.id());
                    });

                auto& canvas = card.emplace<Canvas>("preview_canvas");
                canvas.setHeight(220.0f);
                if (volumeMode) {
                    char stats[128];
                    std::snprintf(stats, sizeof(stats), "3D grid %dx%dx%d",
                                  fluidWorld->volume().resolutionX(),
                                  fluidWorld->volume().resolutionY(),
                                  fluidWorld->volume().resolutionZ());
                    card.emplace<Label>(stats, TextTone::Muted);
                    canvas.setOnDraw([fluidWorld, component](UIContext&, ImDrawList* drawList,
                                                             const ImVec2& origin,
                                                             const ImVec2& size) {
                        drawVolumePreview(fluidWorld->volume(), *component, drawList, origin,
                                          size);
                    });
                } else {
                    char stats[160];
                    std::snprintf(stats, sizeof(stats),
                                  "Sea state %.2f m · %zu waves · shallow %s",
                                  fluidWorld->waves().significantWaveHeight(),
                                  fluidWorld->waves().waves().size(),
                                  component->shallowPhysics ? "on" : "off");
                    card.emplace<Label>(stats, TextTone::Muted);
                    if (component->simulateFluid2D != 0) {
                        canvas.setOnDraw([fluidWorld, component](UIContext&,
                                                                 ImDrawList* drawList,
                                                                 const ImVec2& origin,
                                                                 const ImVec2& size) {
                            draw2DPreview(fluidWorld->fluid2D(), *component, drawList, origin,
                                          size);
                        });
                    } else {
                        canvas.setOnDraw([fluidWorld, component](UIContext&,
                                                                 ImDrawList* drawList,
                                                                 const ImVec2& origin,
                                                                 const ImVec2& size) {
                            drawSurfacePreview(*fluidWorld, *component, drawList, origin, size);
                        });
                    }
                }
            }
        }

        ctx.draw(root);

        bool changed = false;
        if (level != static_cast<int>(component->mode)) {
            const Caffeine::u8 keepPreset = component->preset;
            *component = makeComponent(level);
            component->preset = keepPreset;
            changed = true;
        }
        if (volumeMode && preset != static_cast<int>(component->preset)) {
            applyPresetColors(*component, preset);
            changed = true;
        }
        if (surfaceResolution != static_cast<int>(component->surfaceResolution)) {
            component->surfaceResolution = static_cast<Caffeine::u32>(surfaceResolution);
            changed = true;
        }
        if (simulationResolution != static_cast<int>(component->simulationResolution)) {
            component->simulationResolution = static_cast<Caffeine::u32>(simulationResolution);
            changed = true;
        }
        if (fluidResolution != static_cast<int>(component->fluidResolution)) {
            component->fluidResolution = static_cast<Caffeine::u32>(fluidResolution);
            changed = true;
        }
        if (shallow != (component->shallowPhysics != 0)) {
            component->shallowPhysics = shallow ? 1 : 0;
            changed = true;
        }
        if (preview != (component->previewInEditor != 0)) {
            component->previewInEditor = preview ? 1 : 0;
            changed = true;
        }
        if (renderVolume != (component->renderVolume != 0)) {
            component->renderVolume = renderVolume ? 1 : 0;
            changed = true;
        }
        if (simulateFluid2D != (component->simulateFluid2D != 0)) {
            component->simulateFluid2D = simulateFluid2D ? 1 : 0;
            changed = true;
        }
        if (color[0] != component->albedoR || color[1] != component->albedoG ||
            color[2] != component->albedoB || color[3] != component->albedoA) {
            component->albedoR = color[0];
            component->albedoG = color[1];
            component->albedoB = color[2];
            component->albedoA = color[3];
            changed = true;
        }
        if (std::memcmp(&before, component, sizeof(NaturalFluidComponent)) != 0) {
            changed = true;
        }
        if (changed) Caffeine::Editor::PluginMarkDirty(self->m_host);
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
};

NaturalFluidsPlugin* g_plugin = nullptr;

}  // namespace

extern "C" Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    if (!g_plugin) g_plugin = new NaturalFluidsPlugin(host);
    return g_plugin;
}

extern "C" void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}
