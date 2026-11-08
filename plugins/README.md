# Plugin catalog

Each folder contains the plugin **source code** and a `CMakeLists.txt`. Build against a Caffeine Engine checkout (`CAFFEINE_ROOT`).

## Official plugins

| Plugin | Binary | Folder |
|--------|--------|--------|
| Hello | `hello_plugin` | [hello-plugin](hello-plugin/) |
| Terrain Generator | `terrain_plugin` | [terrain-generator](terrain-generator/) |
| Procedural Tools | `procedural_tools_plugin` | [procedural-tools](procedural-tools/) |
| Post Processing | `post_processing_plugin` | [post-processing](post-processing/) |
| Git | `git_plugin` | [git](git/) |
| AI Development | `ai_development_plugin` | [ai-development](ai-development/) |
| VC&M | `volumetric_clouds_plugin` | [volumetric-clouds](volumetric-clouds/) |

## Build all plugins

```bash
cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build
```

Artifacts land in `build/plugins/*.so`.

## Install into Doppio

Doppio no longer builds or preloads plugins. Copy the artifacts into a plugin
directory and enable only what you need from **Plugins → Plugin Manager**:

```bash
# Per-project plugins
cp build/plugins/*.so /path/to/project/plugins/

# Or user-wide, next to the editor binary
cp build/plugins/*.so /path/to/caffeine/build/plugins/
```

Enabled plugins are remembered per user under `~/.config/caffeine/plugins.cfg`.
Nothing is loaded until you opt in.

## Community plugins

Submit yours via [CONTRIBUTING.md](../CONTRIBUTING.md).

## Compatibility

| doppio-plugins | Caffeine Engine |
|----------------|-----------------|
| `main` | `main` / `feature/editor-plugin-sdk` |

Rebuild plugins after updating the engine.
