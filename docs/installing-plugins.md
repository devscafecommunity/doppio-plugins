# Installing plugins

Doppio discovers plugins at startup by scanning for shared libraries in a `plugins/` directory.

## Default locations

| Platform | Path (relative to Doppio binary) |
|----------|----------------------------------|
| Linux | `./plugins/*.so` |
| Windows | `./plugins/*.dll` |
| macOS | `./plugins/*.dylib` |

Example after a typical Caffeine build:

```
caffeine/build/doppio/
├── doppio                  ← editor executable
└── plugins/
    ├── hello_plugin.so
    ├── terrain_plugin.so
    └── git_plugin.so
```

## Manual install

```bash
cp /path/to/built/my_plugin.so /path/to/doppio/plugins/
```

Restart Doppio or use **Window → Plugins** to reload.

## Build-time bundling (developers)

From the Caffeine CMake configure step:

```bash
cmake -DCAFFEINE_BUNDLE_DEFAULT_PLUGINS=ON ..
cmake --build . --target doppio
```

This copies default plugins into `doppio/plugins/` automatically. Default is **OFF** for a lean IDE distribution.

## Enabling and disabling

1. Open **Window → Plugins**
2. Toggle plugins on/off
3. Disabled plugins are not unloaded from disk but their UI is not registered

## Distribution

For end users, ship a zip with:

```
MyGameEditor/
├── doppio
├── plugins/
│   ├── terrain_plugin.so
│   └── post_processing_plugin.so
└── README.txt
```

Document external dependencies per plugin (Node.js for terrain, `git` for Git plugin, etc.).

## Version compatibility

Plugins are **not** guaranteed ABI-stable across major Caffeine releases. Pin a tested engine version in your plugin README:

```markdown
## Tested with
- Caffeine Engine: commit `abc123` or tag `v0.4.0`
- Doppio: bundled with same release
```
