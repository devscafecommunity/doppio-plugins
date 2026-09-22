# Contributing to doppio-plugins

Thank you for helping grow the Doppio plugin ecosystem.

## What belongs here

| Type | Location | Notes |
|------|----------|-------|
| New community plugin | `plugins/<name>/` | Source + README + CMakeLists.txt |
| Starter / template improvements | `templates/` | Keep minimal and SDK-only |
| Build tooling | `cmake/` | Shared helpers only |
| Documentation | `docs/` | User-facing guides |

**Do not** submit engine core changes here — open those in [caffeine](https://github.com/caffeine-engine/caffeine).

## Plugin checklist

Before opening a pull request, ensure your plugin:

- [ ] Builds with `doppio_add_plugin()` and `CAFFEINE_ROOT` set
- [ ] Exports `CreatePlugin` and `DestroyPlugin` with `extern "C"` and `CAFFEINE_PLUGIN_API`
- [ ] Uses a **unique binary name** (library stem = plugin id, e.g. `my_tool_plugin.so`)
- [ ] Implements `GetName()`, `GetVersion()`, and `GetDescription()`
- [ ] Cleans up in `OnUnload()` (unregister is handled by the host on unload)
- [ ] Includes a `plugins/<name>/README.md` with install steps and dependencies
- [ ] Does not bundle proprietary SDKs or assets without a clear license
- [ ] Is licensed under MPL-2.0 (or compatible — discuss first if different)

## Directory structure for a new plugin

```
plugins/my-plugin/
├── README.md
├── CMakeLists.txt
└── src/
    └── main.cpp
```

Example `CMakeLists.txt`:

```cmake
include(DoppioPlugin)

doppio_add_plugin(my_plugin
    OUTPUT_NAME my_plugin
    SOURCES src/main.cpp)
```

Add your subdirectory to the root `CMakeLists.txt` when ready for CI.

## Coding guidelines

- Prefer **`caffeine/plugin/PluginAPI.hpp`** over internal engine headers when possible
- Plugins that need ECS or editor internals may include `src/` headers from `CAFFEINE_ROOT` — this couples you to engine versions; document the minimum Caffeine version in your README
- Use `m_host->logInfo` / `logError` instead of `printf` for user-visible messages
- Guard ImGui code with `#ifdef CF_HAS_IMGUI`

## Services and cross-plugin APIs

If your plugin exposes capabilities for others, register named services in `OnLoad()`:

```cpp
m_host->registerService(m_host->editorContext, GetName(), "myplugin.doThing", &handler);
```

Document request/response structs in your README and consider adding constants to a shared header under `include/` if multiple plugins need them.

## Pull request process

1. Fork and create a feature branch
2. Add your plugin under `plugins/<name>/`
3. Verify the plugin loads in Doppio without errors
4. Open a PR with:
   - What the plugin does
   - Screenshots or short demo (if UI)
   - Tested Caffeine / Doppio version
   - Any external runtime dependency (Node.js, `git`, etc.)

## Code of conduct

Be respectful and constructive. We follow the same standards as the Caffeine Engine project.

## Questions

Open a [GitHub Discussion](https://github.com/caffeine-engine/doppio-plugins/discussions) or an issue with the `question` label.
