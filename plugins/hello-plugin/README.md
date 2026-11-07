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
# From this repository
cmake -B build \
  -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build --target hello_plugin
cp build/plugins/hello_plugin.so /path/to/caffeine/build/plugins/
```

Then enable it in **Plugins → Plugin Manager**.

## Usage

1. Launch Doppio
2. **Plugins → Plugin Manager** → enable **Hello Plugin**
3. Open the **Hello Plugin** panel
4. Click **Log to console**

## Source

| Location | Path |
|----------|------|
| This repo | `plugins/hello-plugin/src/main.cpp` |
| This repo (template) | `templates/hello-plugin/` |

## Dependencies

None beyond Caffeine Plugin SDK + ImGui.

## API demonstrated

- `IPlugin::OnLoad` / `OnUnload` / `OnUpdate`
- `registerPanel`, `registerMenuAction`
- `CreatePlugin` / `DestroyPlugin` exports
