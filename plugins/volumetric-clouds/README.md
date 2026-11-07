# VC&M — Volumetric Clouds & Mist

> **Binary:** `volumetric_clouds_plugin.so`  
> **Panel:** VC&M  
> **License:** MPL-2.0

Procedural volumetric clouds and ground mist for Caffeine scenes. The plugin
authors a weather profile; the engine ray-marches the cloud layer in the sky pass.

## Features

- Weather model driven by **humidity**, **instability**, **wind** and **seed**.
- Presets: Fair weather, Cumulus, Overcast, Storm, Misty morning, Alpine, Clear sky.
- Cloud layer: coverage, density, base/top altitude, detail, drift and sun scatter.
- Ground mist: density, height and falloff.
- Quality presets: Low (8 steps), Medium (16), High (24).
- Adds an **Inspector drawer** for `VolumetricCloudsComponent`.

## How it works

`Evaluate()` maps the weather model to the component:

| Input | Effect |
|-------|--------|
| Humidity | Cloud coverage, density, lower base, thicker layer, more mist |
| Instability | Detail octaves, vertical development, storm cells |
| Wind | Drift direction and speed of the noise field |
| Seed | Shifts the procedural field |

The engine (`sky.frag`) ray-marches the volume between `baseAltitude` and
`topAltitude`, samples 3D value-noise FBM, marches toward the sun for
self-shadowing, and blends ground mist near the horizon.

## Install

```bash
cmake -S doppio-plugins -B doppio-plugins/build \
  -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build doppio-plugins/build --target volumetric_clouds_plugin
cp build/plugins/volumetric_clouds_plugin.so /path/to/caffeine/build/plugins/
```

Enable **VC&M** in **Plugins → Plugin Manager**, then open **Effects → Volumetric
Clouds & Mist**.

## Usage

1. Select an entity (or let the plugin create a dedicated atmosphere entity).
2. Pick a preset, tune humidity/instability and press **Generate**.
3. Switch the viewport to **3D** and look toward the sky.
4. Tune the cloud layer and mist directly; the scene rebuilds on each change.

## Performance

High quality uses 24 ray-march steps plus 4 sun-march steps. Start with **Medium**
for editing and use **High** for captures. The cloud pass only runs when the sky is
visible.
