# Natural Fluids

> **Binary:** `natural_fluids_plugin.so`  
> **Panel:** Natural Fluids  
> **Component:** `NaturalFluidComponent` (`caffeine/environment/NaturalFluids.hpp`)  
> **License:** MPL-2.0

Water surfaces and real Eulerian fluids for Doppio scenes. The plugin authors four
product levels; the engine owns the simulators and renders the generated meshes.

## The four levels

| Level | What it is |
|-------|------------|
| **Flat water** | A perfectly flat animated plane with a transparent PBR water material. |
| **Wind + physics** | Analytic Gerstner wind waves **plus** a conservative shallow-water solver (drops, impulses, wind stress, terrain bed, volume conservation). |
| **Realistic water** | A broad directional Gerstner spectrum with finite-depth dispersion, choppiness, Jacobian whitecap foam and a low-roughness water material. |
| **Fluid volume** | A real 3D Eulerian fluid (Stable Fluids): semi-Lagrangian advection, Jacobi pressure projection, buoyancy, vorticity confinement. Materials: water, fire, fog, smoke. |

## What is really simulated

- `WaveField` — deterministic Gerstner waves; the same parameters drive the CPU
  sampler, the generated mesh and the foam preview.
- `ShallowWater` — face-staggered conservative shallow-water solver. A closed basin
  keeps its volume; drops, impulse and wind inject momentum.
- `EulerFluid2D` / `FluidVolume` — incompressible fluid solvers with density and
  temperature. Fire rises because of temperature buoyancy; smoke and fog are
  transported by the same velocity field.
- The engine system (`Scene::tickNaturalFluids`) rebuilds the render mesh every
  frame from the simulation, with smooth normals and UVs, and uploads it through
  the existing `MeshGeometryComponent` path.

The component is a POD and is serialized with the scene. Generated meshes are not
serialized: they are rebuilt from the component on load.

## Panel

- Create a realistic water surface, a flat one, a wind + physics one or a fluid
  volume.
- Add `Natural Fluids` to any selected entity.
- Edit size, water level, wind, amplitude, choppiness, foam threshold, mesh and
  simulation resolution, time scale and material.
- Fluid volume: material preset, source rate/radius/heat, fluid wind, emission and
  density-voxel rendering.
- Inject drops/impulses/puffs and reset the simulation.
- Live preview: water height/foam map, 2D density slice, or two 3D slices.

## Rendering notes

- Water uses the existing PBR scene shader with a transparent, low-roughness water
  material. A dedicated water shader (screen-space refraction, planar SSR, foam
  texture) is the next rendering step.
- Fluid volumes are visualized as density voxels so the real 3D field is visible
  without a new render pass. A raymarch volume pass can replace the voxels later.
- Volume voxels never cast solid shadows.

## Build

```bash
cmake -S doppio-plugins -B doppio-plugins/build \
  -DCAFFEINE_ROOT=/path/to/caffeine \
  -DIMGUI_DIR=/path/to/caffeine/build/_deps/imgui-src
cmake --build doppio-plugins/build --target natural_fluids_plugin
cp doppio-plugins/build/plugins/natural_fluids_plugin.so /path/to/caffeine/build/plugins/
```

Enable **Natural Fluids** in **Plugins → Plugin Manager**.

The simulation core is shared with the engine:

- `src/fluid/WaveField.*`
- `src/fluid/ShallowWater.*`
- `src/fluid/EulerFluid.*`
- `src/fluid/FluidVolume.*`
- `src/fluid/FluidWorld.*`

The standalone lab lives in the Stratosphere project at
`native/fluid-lab`; it builds the same core against `libcaffeine-core.a` and runs
48 self-checks plus OBJ/PPM exports.
