# Getting started

This guide walks through building and installing your first Doppio plugin.

## Prerequisites

1. **Caffeine Engine** cloned and built with Doppio enabled (SDL3 + ImGui)
2. **CMake 3.20+** and a C++20 compiler
3. **doppio-plugins** cloned (this repository)

## Build the hello template

```bash
export CAFFEINE_ROOT=/path/to/caffeine
export IMGUI_DIR=$CAFFEINE_ROOT/build/_deps/imgui-src

cd doppio-plugins
cmake -B build -DCAFFEINE_ROOT=$CAFFEINE_ROOT -DIMGUI_DIR=$IMGUI_DIR
cmake --build build
```

Output: `build/plugins/hello_plugin.so` (Linux) or `hello_plugin.dll` (Windows).

## Install

Doppio scans the `plugins/` directory next to its executable:

```bash
mkdir -p $CAFFEINE_ROOT/build/doppio/plugins
cp build/plugins/hello_plugin.so $CAFFEINE_ROOT/build/doppio/plugins/
```

Launch Doppio:

```bash
$CAFFEINE_ROOT/build/doppio/doppio
```

Open **Window → Plugins**, enable **Hello Plugin**, then find the **Hello Plugin** panel.

## Create your own plugin

```bash
cp -r templates/hello-plugin plugins/my-plugin
# edit plugins/my-plugin/src/main.cpp
# add add_subdirectory(plugins/my-plugin) to root CMakeLists.txt
cmake --build build
```

See [writing-a-plugin.md](writing-a-plugin.md) for API details.

## Official plugins

All official plugins ship with **source code** under `plugins/`:

```bash
cmake -B build -DCAFFEINE_ROOT=$CAFFEINE_ROOT -DIMGUI_DIR=$IMGUI_DIR \
  -DDOPPIO_PLUGINS_BUILD_TEMPLATE=OFF
cmake --build build
cp build/plugins/*.so $CAFFEINE_ROOT/build/doppio/plugins/
```

See the [plugin catalog](../plugins/README.md) for per-plugin details.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Plugin not listed | Check file is in `doppio/plugins/` and has `.so` / `.dll` extension |
| `CreatePlugin` not found | Build with `CAFFEINE_PLUGIN_EXPORTS=1`; use `doppio_add_plugin()` |
| ImGui headers missing | Set `-DIMGUI_DIR=.../imgui-src` from Caffeine's CMake fetch |
| Crash on load | Mismatched Caffeine version — rebuild plugin against same tree |
| Panel empty | Ensure `CF_HAS_IMGUI=1` at compile time |
