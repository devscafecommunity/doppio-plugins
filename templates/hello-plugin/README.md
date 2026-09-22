# Hello Plugin (template)

Minimal Doppio plugin — starting point for new projects.

## What it does

- Registers a **Hello Plugin** ImGui panel
- Adds menu action **Help → Hello from Plugin**
- Logs a message to the editor console on button click

## Build

From the `doppio-plugins` root:

```bash
cmake -B build \
  -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build build
```

## Install

```bash
cp build/plugins/hello_plugin.so /path/to/doppio/plugins/
```

## Copy as new plugin

```bash
cp -r templates/hello-plugin plugins/my-plugin
# rename target in CMakeLists.txt and class in main.cpp
```

## Source location

This template mirrors `apps/plugins/hello_plugin/` in the Caffeine monorepo.
