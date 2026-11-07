#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginServices.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "ecs/TerrainComponents.hpp"
#include "TerrainBridge.hpp"

#include <cstring>
#include <iostream>

namespace {

const Caffeine::Editor::PluginHostApi* hostFrom(void* context) {
    return static_cast<const Caffeine::Editor::PluginHostApi*>(context);
}

bool terrainGenerateUltra(void* editorContext, const void* request, std::size_t requestSize,
                          void* response, std::size_t responseSize) {
    (void)response;
    (void)responseSize;
    if (!editorContext || !request ||
        requestSize < sizeof(Caffeine::Editor::TerrainGenerateUltraRequest)) {
        return false;
    }
    const auto* host = hostFrom(editorContext);
    Caffeine::ECS::World* world = Caffeine::Editor::PluginActiveWorld(host);
    if (!world) return false;

    const auto* req = static_cast<const Caffeine::Editor::TerrainGenerateUltraRequest*>(request);
    Caffeine::ECS::Entity entity(req->entityId, world);
    if (!entity.isValid() || !world->has<Caffeine::ECS::TerrainComponent>(entity)) return false;

    std::string error;
    const bool ok = Caffeine::Editor::TerrainBridge::generateUltraRealistic(
        *world, entity, req->preset, req->seed, req->worldSizeMeters, req->gridSize, error);
    if (!ok) {
        std::cerr << "[TerrainPlugin] terrain.generateUltra: " << error << '\n';
        return false;
    }
    Caffeine::Editor::PluginMarkDirty(host);
    return true;
}

bool terrainImportHeightmap(void* editorContext, const void* request, std::size_t requestSize,
                            void* response, std::size_t responseSize) {
    (void)response;
    (void)responseSize;
    if (!editorContext || !request ||
        requestSize < sizeof(Caffeine::Editor::TerrainImportHeightmapRequest)) {
        return false;
    }
    const auto* host = hostFrom(editorContext);
    Caffeine::ECS::World* world = Caffeine::Editor::PluginActiveWorld(host);
    if (!world) return false;

    const auto* req = static_cast<const Caffeine::Editor::TerrainImportHeightmapRequest*>(request);
    Caffeine::ECS::Entity entity(req->entityId, world);
    if (!entity.isValid()) return false;

    std::string error;
    const bool ok = Caffeine::Editor::TerrainBridge::importHeightmap(
        *world, entity, req->heightmapPath, req->worldSizeX, req->worldSizeZ, error);
    if (!ok) {
        std::cerr << "[TerrainPlugin] terrain.importHeightmap: " << error << '\n';
        return false;
    }
    Caffeine::Editor::PluginMarkDirty(host);
    return true;
}

}  // namespace

extern "C" void caffeine_terrain_plugin_register_services(
    const Caffeine::Editor::PluginHostApi* host) {
    if (!host || !host->registerService) return;
    host->registerService(host->editorContext, "Terrain Generator",
                          Caffeine::Editor::kServiceTerrainGenerateUltra, &terrainGenerateUltra);
    host->registerService(host->editorContext, "Terrain Generator",
                          Caffeine::Editor::kServiceTerrainImportHeightmap, &terrainImportHeightmap);
}
