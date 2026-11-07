#pragma once

#include "core/Types.hpp"
#include "ecs/Entity.hpp"
#include "ecs/World.hpp"

#include <string>

namespace Caffeine::PhotonAOBake {

struct Settings {
    u32 resolution = 512;
    u32 rays = 16;
    f32 maxDistance = 2.5f;
    f32 bias = 0.02f;
};

struct Result {
    bool ok = false;
    std::string error;
    std::string ormPath;
    std::string materialPath;
    u32 coveredTexels = 0;
    u32 triangles = 0;
};

/// Bakes static ambient occlusion for `entity` into an ORM texture and assigns
/// it to the entity material. The scene's static meshes are used as occluders.
/// Runs on the calling thread (editor UI); intended for editor-time bakes.
Result bake(ECS::World& world, ECS::Entity entity, const Settings& settings,
            const std::string& projectRoot);

}  // namespace Caffeine::PhotonAOBake
