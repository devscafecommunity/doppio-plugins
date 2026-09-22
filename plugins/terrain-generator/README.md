# Terrain Generator Plugin

> **Binary:** `terrain_plugin.so`  
> **Panel:** Terrain Generator  
> **License:** MPL-2.0

Ultra-realistic procedural terrain for Caffeine scenes. Generation runs outside the editor core via a Node.js pipeline with domain warping and geomorphological PDEs.

## Features

- Presets: `default`, `alpine`, `mesa`, `volcanic`, `glacial`, `blend`
- Configurable world size, grid resolution, and seed
- Heightmap import into `TerrainComponent`
- Rebuilds render mesh, LOD, and collision mesh

## Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| Caffeine Engine | latest `main` | `TerrainComponent` + importer |
| Node.js | 18+ | Runs terrain algorithm |
| npm | | `npm install` in algorithm folder (first run) |

Algorithm assets: `assets/general-ultra-realistic-terrain-algorithm/` in the Caffeine repo.

## Install

```bash
cmake --build /path/to/caffeine/build --target terrain_plugin
cp build/plugins/terrain_plugin.so build/doppio/plugins/
```

## Usage

1. Create or select an entity with **Terrain** component
2. Open **Terrain Generator** panel
3. Choose preset, size, grid, seed → **Generate**
4. Wait for Node export → automatic import

## Services exposed

| Service | Purpose |
|---------|---------|
| `terrain.generateUltra` | Run full generation pipeline |
| `terrain.importHeightmap` | Import existing heightmap file |

See [service registry](../../docs/service-registry.md).

## Source

```
apps/plugins/terrain_plugin/
├── main.cpp
├── TerrainBridge.cpp
└── TerrainPluginServices.cpp
```

## Further reading

- [Caffeine terrain plugin docs](https://github.com/caffeine-engine/caffeine/blob/main/docs/terrain/terrain-plugin.md)
