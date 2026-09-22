# Git Plugin

> **Binary:** `git_plugin.so`  
> **Panel:** Git Panel  
> **License:** MPL-2.0  
> **Bundled by default:** No (optional)

Git workflow integration for Caffeine projects. Requires the `git` CLI in `PATH`.

## Features

- Repository detection and `git init`
- **Changes** — status, stage/unstage per file or all
- **Commit** — message + commit
- **Branches** — checkout, create
- **Remotes** — fetch, pull, push
- **History** — recent log
- **Stash** — push, pop, list
- Per-file diff in output panel

## Requirements

| Dependency | Notes |
|------------|-------|
| `git` | 2.30+ recommended |
| Open Caffeine project | Uses project root from active scene |

## Install

```bash
cmake --build /path/to/caffeine/build --target git_plugin
cp build/plugins/git_plugin.so build/doppio/plugins/
```

## Usage

- Menu: **Version Control → Git Panel**
- Or enable in **Window → Plugins**

Project root is resolved via `PluginHostApi::getProjectRootPath` (parent directory of `scenes/`).

## Source

```
apps/plugins/git_plugin/
├── main.cpp
└── GitRunner.cpp
```

## Security note

The plugin executes `git` subprocesses with user privileges. Only use with projects you trust.

## Further reading

- [Git plugin docs](https://github.com/caffeine-engine/caffeine/blob/main/docs/editor/git-plugin.md)
