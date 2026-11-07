# Installing plugins

Doppio discovers shared libraries in a few well-known `plugins/` directories but
**loads none of them at startup**. Discovered libraries are listed in the Plugin
Manager and stay disabled until enabled.

## Discovery locations

| Scope | Path |
|-------|------|
| Project | `<project-root>/plugins/*.so` (`*.dll` on Windows) |
| Doppio build | `<caffeine-build>/plugins/*.so` |
| Next to the binary | `<doppio-binary-dir>/plugins/*.so` |

Example after a typical Caffeine build:

```
caffeine/build/
├── doppio
└── plugins/
    ├── hello_plugin.so
    ├── terrain_plugin.so
    └── git_plugin.so
```

## Manual install

```bash
cp /path/to/built/my_plugin.so /path/to/doppio/plugins/
```

Restart Doppio or press **Refresh Directory** in **Plugins → Plugin Manager**.

## Enabling and disabling

1. Open **Plugins → Plugin Manager**.
2. Tick **Enabled** next to a library to load it and register its panels, menus,
   drawers and services.
3. Untick it (or press **Unload**) to remove its contributions and unload the
   library.

The enable list is written to `~/.config/caffeine/plugins.cfg`
(`$XDG_CONFIG_HOME/caffeine/plugins.cfg` when set, `%APPDATA%/Caffeine` on Windows).
Each line is a library stem; comments start with `#`.

```
# enabled plugins
terrain_plugin
git_plugin
```

## Distribution

Ship the `.so`/`.dll` files and a short readme. Do not modify the engine tree.
For a team default, distribute a `plugins.cfg` that developers copy into their user
config; each user can still disable plugins locally.

## Compatibility

Plugins are compiled against the public headers in `include/caffeine/plugin/`. Keep
the ABI aligned with the engine; a plugin built for a different ABI may fail to load
or crash. Use the same compiler and standard library as Doppio on Linux.
