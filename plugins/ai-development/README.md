# AI Development Plugin

> **Binary:** `ai_development_plugin.so`  
> **Panel:** AI Development  
> **License:** MPL-2.0

Author NPC behaviour without touching engine internals. The plugin adds navigation
volumes, nav agents, patrol points, and ready-to-edit Lua behaviour templates to the
selected entity.

## Features

- Add or edit a `NavVolume` grid directly from the panel.
- Configure `NavAgent` behaviour: idle, patrol, follow, or scripted.
- Generate Lua templates for the selected behaviour under the project's scripts folder.
- Uses the public `caffeine/navigation/NavigationApi.hpp` and `caffeine/plugin/` SDK only.

## Build

```bash
cmake -B build -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build --target ai_development_plugin
```

Copy `build/plugins/ai_development_plugin.so` into the Doppio plugins directory and
enable it from **Plugins → Plugin Manager**.
