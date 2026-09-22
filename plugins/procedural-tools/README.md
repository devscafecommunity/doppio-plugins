# Procedural Tools Plugin

> **Binary:** `procedural_tools_plugin.so`  
> **Panel:** Procedural Tools  
> **License:** MPL-2.0

Modular procedural streaming toolkit — terrain height profiles, structure spawners, and Lua directors for infinite worlds.

## Features

- **Streaming setup** — chunk size, view radius, seed
- **Terrain profiles** — `flat`, `hills`, `fbm`
- **Structure spawners** — `none`, `markers`, `rooms`, `track`
- **Lua directors** — custom `scripts/procedural/*.lua`
- **Benchmarks** — exploration, backrooms, endless race (reference scenes)
- Runtime API: `caffeine.procedural.*` in play mode

## Requirements

| Dependency | Notes |
|------------|-------|
| Caffeine Engine | Procedural ECS systems + Lua runtime |
| Lua scripts | Installed via **Install Lua scripts** button |

Script templates: `assets/plugins/procedural/scripts/` in Caffeine.

## Install

```bash
cmake --build /path/to/caffeine/build --target procedural_tools_plugin
cp build/plugins/procedural_tools_plugin.so build/doppio/plugins/
```

## Usage

1. Open **Procedural Tools** panel
2. Configure streaming parameters
3. **Install Lua scripts** (first time)
4. **Create Streaming Setup** → spawns director + terrain entities
5. Press **Play** — chunks stream around the camera

## Services exposed

| Service | Purpose |
|---------|---------|
| `procedural.installScripts` | Copy template Lua to project |
| `procedural.createStreamingSetup` | Spawn director + terrain |
| `procedural.loadBenchmark` | Load reference benchmark scene |

## Source

```
apps/plugins/procedural_tools_plugin/
├── main.cpp
├── ProceduralBridge.cpp
└── ProceduralPluginServices.cpp
```

## Further reading

- [Procedural tools docs](https://github.com/caffeine-engine/caffeine/blob/main/docs/terrain/procedural-tools-plugin.md)
