# doppio-plugins

Official and community **plugins for [Doppio](https://github.com/caffeine-engine/caffeine)** — the editor for the Caffeine Engine.

Doppio is intentionally lean: terrain tools, post-processing UI, procedural streaming, Git integration, and other feature panels ship as **dynamic libraries** (`.so` / `.dll`) loaded at runtime from a `plugins/` folder next to the editor binary.

This repository is the public home for:

- **Plugin catalog** — what each plugin does and how to install it
- **Starter template** — minimal plugin you can copy and extend
- **CMake helpers** — build against the Caffeine Plugin SDK
- **Community contributions** — share your own editor extensions

## Quick start

### 1. Build Doppio

You need a working [Caffeine Engine](https://github.com/caffeine-engine/caffeine) checkout with Doppio built:

```bash
git clone https://github.com/caffeine-engine/caffeine.git
cd caffeine && mkdir build && cd build
cmake .. && cmake --build . --target doppio
```

### 2. Build a plugin from this repo

```bash
git clone https://github.com/caffeine-engine/doppio-plugins.git
cd doppio-plugins
cmake -B build \
  -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build
```

### 3. Install into Doppio

Copy the built `.so` / `.dll` into Doppio's plugins directory:

```bash
cp build/plugins/*.so /path/to/caffeine/build/doppio/plugins/
```

Restart Doppio (or use **Window → Plugins** to enable the plugin).

## Repository layout

```
doppio-plugins/
├── README.md                 ← you are here
├── CONTRIBUTING.md           ← how to submit a plugin
├── cmake/
│   └── DoppioPlugin.cmake    ← shared CMake helper
├── docs/
│   ├── getting-started.md
│   ├── writing-a-plugin.md
│   ├── installing-plugins.md
│   └── service-registry.md
├── templates/
│   └── hello-plugin/         ← minimal copy-paste starter
└── plugins/                  ← official plugin source + CMakeLists.txt each
    ├── README.md
    ├── hello-plugin/src/
    ├── terrain-generator/src/
    ├── procedural-tools/src/
    ├── post-processing/src/
    └── git/src/
```

## Official plugin catalog

| Plugin | Binary | Description |
|--------|--------|-------------|
| [Hello Plugin](plugins/hello-plugin/) | `hello_plugin` | Minimal SDK example |
| [Terrain Generator](plugins/terrain-generator/) | `terrain_plugin` | Ultra-realistic procedural terrain (Node.js pipeline) |
| [Procedural Tools](plugins/procedural-tools/) | `procedural_tools_plugin` | Modular streaming + Lua directors |
| [Post Processing](plugins/post-processing/) | `post_processing_plugin` | Post-process stack UI for cameras |
| [Git](plugins/git/) | `git_plugin` | Optional Git workflow panel |

Each plugin folder contains `src/` and `CMakeLists.txt`. Build everything from this repo:

```bash
cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build
cp build/plugins/*.so /path/to/caffeine/build/doppio/plugins/
```

The Caffeine monorepo still mirrors the same sources under `apps/plugins/` for in-tree builds.

## Writing your own plugin

1. Copy `templates/hello-plugin/` to `plugins/your-plugin/`
2. Implement `Caffeine::Editor::IPlugin` and export `CreatePlugin` / `DestroyPlugin`
3. Register panels, menus, component drawers, or services via `PluginHostApi`
4. Build with `doppio_add_plugin()` from `cmake/DoppioPlugin.cmake`

See [docs/writing-a-plugin.md](docs/writing-a-plugin.md) for the full guide.

## Plugin SDK reference

The public C ABI is defined in the Caffeine tree:

- `include/caffeine/plugin/PluginAPI.hpp` — host callbacks, `IPlugin` interface
- `include/caffeine/plugin/PluginServices.hpp` — well-known service names and request structs

Upstream documentation: [Caffeine Plugin SDK](https://github.com/caffeine-engine/caffeine/blob/main/docs/editor/plugin-sdk.md)

## Requirements

| Requirement | Notes |
|-------------|-------|
| Caffeine Engine | Matching branch / tag recommended |
| C++20 compiler | GCC 11+, Clang 14+, MSVC 2022 |
| CMake 3.20+ | |
| ImGui | Fetched by Caffeine build; pass `IMGUI_DIR` when building standalone |

## License

[MPL-2.0](LICENSE) — same as Caffeine Engine. Plugin authors keep copyright on their contributions; see [CONTRIBUTING.md](CONTRIBUTING.md).

## Links

- [Caffeine Engine](https://github.com/caffeine-engine/caffeine)
- [Plugin SDK docs](https://github.com/caffeine-engine/caffeine/blob/main/docs/editor/plugin-sdk.md)
- [Engine pillars roadmap](https://github.com/caffeine-engine/caffeine/blob/main/docs/plans/2026-09-22-engine-pillars-roadmap.md)
