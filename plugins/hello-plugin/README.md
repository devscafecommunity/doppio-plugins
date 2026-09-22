# Hello Plugin

> **Binary:** `hello_plugin.so` / `hello_plugin.dll`  
> **Type:** SDK example  
> **License:** MPL-2.0

The smallest useful Doppio plugin — use it to verify your build pipeline and learn the API.

## Features

- Custom ImGui panel (**Hello Plugin**)
- Menu entry: **Help → Hello from Plugin**
- Console logging via `PluginHostApi::logInfo`

## Install

```bash
# From Caffeine monorepo
cmake --build build --target hello_plugin
cp build/plugins/hello_plugin.so build/doppio/plugins/

# Or from doppio-plugins template
cmake -B build -DCAFFEINE_ROOT=../caffeine -DIMGUI_DIR=../caffeine/build/_deps/imgui-src
cmake --build build
cp build/plugins/hello_plugin.so /path/to/doppio/plugins/
```

## Usage

1. Launch Doppio
2. **Window → Plugins** → enable **Hello Plugin**
3. Open the **Hello Plugin** panel
4. Click **Log to console**

## Source

| Location | Path |
|----------|------|
| Caffeine monorepo | `apps/plugins/hello_plugin/main.cpp` |
| This repo (template) | `templates/hello-plugin/` |

## Dependencies

None beyond Caffeine Plugin SDK + ImGui.

## API demonstrated

- `IPlugin::OnLoad` / `OnUnload` / `OnUpdate`
- `registerPanel`, `registerMenuAction`
- `CreatePlugin` / `DestroyPlugin` exports
