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

## Build all plugins

```bash
cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build
```

Artifacts land in `build/plugins/*.so`.

## Install into Doppio

```bash
cp build/plugins/*.so /path/to/caffeine/build/plugins/
# or next to the editor binary:
cp build/plugins/*.so /path/to/caffeine/build/doppio/plugins/
```

## Community plugins

Submit yours via [CONTRIBUTING.md](../CONTRIBUTING.md).

## Compatibility

| doppio-plugins | Caffeine Engine |
|----------------|-----------------|
| `main` | `main` / `feature/editor-plugin-sdk` |

Rebuild plugins after updating the engine.
