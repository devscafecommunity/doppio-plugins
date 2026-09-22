# Plugin catalog

All plugins in this repository. Each folder has a dedicated README with install steps and dependencies.

## Official plugins

Maintained by the Caffeine team. Source currently in [caffeine/apps/plugins](https://github.com/caffeine-engine/caffeine/tree/main/apps/plugins).

| Plugin | Binary | Status | Docs |
|--------|--------|--------|------|
| Hello | `hello_plugin` | Stable | [hello-plugin](hello-plugin/) |
| Terrain Generator | `terrain_plugin` | Stable | [terrain-generator](terrain-generator/) |
| Procedural Tools | `procedural_tools_plugin` | Stable | [procedural-tools](procedural-tools/) |
| Post Processing | `post_processing_plugin` | Stable | [post-processing](post-processing/) |
| Git | `git_plugin` | Optional | [git](git/) |

## Community plugins

_Submit yours via [CONTRIBUTING.md](../CONTRIBUTING.md)._

| Plugin | Author | Description |
|--------|--------|-------------|
| _—_ | | |

## Build all official plugins (from Caffeine)

```bash
cd /path/to/caffeine/build
cmake --build . --target \
  hello_plugin \
  terrain_plugin \
  procedural_tools_plugin \
  post_processing_plugin \
  git_plugin
cp plugins/*.so doppio/plugins/
```

## Compatibility matrix

| doppio-plugins | Caffeine Engine |
|----------------|-----------------|
| `main` | `main` / `feature/editor-plugin-sdk` |

Always rebuild plugins after updating the engine.
