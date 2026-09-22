# Post Processing Plugin

> **Binary:** `post_processing_plugin.so`  
> **Panel:** Post Processing  
> **License:** MPL-2.0

Editor UI for the modular post-process stack on camera entities.

## Features

- **Post Processing** panel for stack editing
- Menu: **Effects → Add Post Process to Camera**
- Custom **Inspector** drawer for `PostProcess` component
- Per-effect modules with presets (bloom, color grade, vignette, etc.)
- Optional Lua control and benchmarks

## Requirements

| Dependency | Notes |
|------------|-------|
| Caffeine Engine | `PostProcessComponent`, runtime stack |
| GPU viewport | Effects apply in play / GPU preview |

## Install

```bash
cmake --build /path/to/caffeine/build --target post_processing_plugin
cp build/plugins/post_processing_plugin.so build/doppio/plugins/
```

## Usage

1. Select a camera entity (or use menu to add component)
2. Open **Post Processing** panel or use Inspector drawer
3. Add / reorder / configure effect modules
4. Play mode applies the stack at runtime

## Source

```
apps/plugins/post_processing_plugin/main.cpp
```

## Notes

- ECS data (`PostProcessComponent`) lives in the **engine core**; this plugin owns **editor UI only**
- Without the plugin loaded, the component still serializes but has no custom Inspector

## Further reading

- [Caffeine Plugin SDK](https://github.com/caffeine-engine/caffeine/blob/main/docs/editor/plugin-sdk.md)
