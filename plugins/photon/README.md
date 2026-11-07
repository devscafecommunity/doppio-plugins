# Photon — Hybrid Radiance Cascade GI

> **Binary:** `photon_plugin.so`  
> **Panel:** Photon  
> **License:** MPL-2.0

Photon is a real-time global-illumination system for scenes without hardware
ray tracing. This v0.1 plugin authors the engine's `PhotonGIComponent`; the
engine runs the screen-space cascade pass after TAA and before the final
composite.

## v0.1 scope

| Part | Status |
|------|--------|
| Screen-space near field | Implemented: K exponentially growing intervals traced against the depth buffer |
| Deterministic merge | Implemented: front-to-back radiance accumulation |
| Far field | Sky/ambient fallback when a ray leaves the screen or misses |
| Multi-bounce feedback | Not yet (uses same-frame direct radiance) |
| World-anchored probe field | Not yet |
| Glossy reflection pass | Not yet |

The full Photon specification is a design target; this plugin is the first
functional near-field slice. It is honest about the missing far-field probes
and the temporal denoiser.

## Presets

| Preset | Quality | Cascades | Radius |
|--------|---------|----------|--------|
| Performance | Low | 3 | 10 m |
| Balanced | Medium | 4 | 14 m |
| High | High | 5 | 22 m |
| Cinematic | High | 6 | 32 m |

## Parameters

- **Intensity** — indirect-light multiplier.
- **Cascades** — number of exponentially growing distance intervals.
- **Radius** — near-field reach.
- **Thickness** — depth tolerance for accepting a screen hit.
- **Sky intensity** — far-field fallback when rays leave the screen.
- **Ambient** — constant bounce term.
- **Tint** — albedo approximation applied to gathered radiance.
- **Debug** — lit scene, GI only, or fallback only.

## Install

The engine build compiles all plugins automatically:

```bash
cmake --build build -j
```

Enable **Photon** in **Plugins → Plugin Manager**, then use
**Plugins → Create Photon GI** or **View → Photon Global Illumination**.

## Requirements

- A scene with a Camera 3D and depth-tested geometry.
- The Photon pass runs in the same post-process stack as SSAO/TAA/bloom, so it
  works in Doppio and in the built runtime.

## Known limitations

- No temporal denoiser in v0.1: low quality presets show noise.
- No world-anchored radiance probes yet: off-screen light uses a constant
  sky/ambient fallback.
- No multi-bounce feedback; indirect light is single-bounce.
- Screen-space occlusion can leak on thin geometry; tune **Thickness**.
