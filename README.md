# sourceLab

[![CI (main)](https://github.com/DSA-NO/sourceLab/actions/workflows/ci.yml/badge.svg?branch=main&event=push)](https://github.com/DSA-NO/sourceLab/actions/workflows/ci.yml?query=branch%3Amain+event%3Apush)

This project is a Geant4 application focused on simulating radioactive and source-driven particle emission using a GPS-based primary generator setup.

## Start Here (new users)

Use this as the default onboarding path. It simulates a clean shell, clones from GitHub, installs micromamba if needed, and runs the app.

```bash
env -i HOME="$HOME" USER="$USER" TERM="${TERM:-xterm-256color}" LANG="${LANG:-C.UTF-8}" PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" bash --noprofile --norc

git clone https://github.com/DSA-NO/sourceLab.git
cd sourceLab
./scripts/install-micromamba.sh
export PATH="$HOME/.local/bin:$PATH"
./scripts/run-micromamba.sh build
./scripts/run-micromamba.sh run
```

This catches onboarding issues such as missing shell init, stale local Geant4 settings, or path-dependent macro/runtime assumptions.

## Quick start (already cloned)

If you already have the repository checked out, use this shorter path:

```bash
cd sourceLab
./scripts/install-micromamba.sh
export PATH="$HOME/.local/bin:$PATH"
./scripts/run-micromamba.sh build
./scripts/run-micromamba.sh run
```

This creates or reuses the `geant4-sourceLab` environment from `environment.yml`, builds the app, and runs it in batch mode by default.

If you want an interactive shell inside the environment:

```bash
./scripts/run-micromamba.sh shell
```

You can also pass a macro directly:

```bash
./scripts/run-micromamba.sh run -b run.mac
```

There are also simple smoke-test macro examples:

```bash
./scripts/run-micromamba.sh run -b run-simple.mac
./scripts/run-micromamba.sh run -v vis-simple.mac
```

The default batch and visual macros configure Cs-137 ions at rest, sampled uniformly throughout the Cs-137 cylinder:

- `macros/source-cs137.mac`: Cs-137 identity and energy
- `macros/position-cs137-cylinder.mac`: cylinder position distribution
- `macros/source.mac`: compatibility alias for `source-cs137.mac`

For an interactive visualization session (use `/run/beamOn 5` to generate events):

```bash
./scripts/run-micromamba.sh run -v init_vis.mac
```

Use `-p option4|livermore|penelope` to select the EM sub-model. For example:

```bash
./scripts/run-micromamba.sh run -p livermore -b run.mac
```

Radioactive decay physics is enabled by default. Use `-r on|off` to enable or disable it; with `-r off`, Cs-137 ions will not undergo radioactive decay. For example:

```bash
./scripts/run-micromamba.sh run -r off -b run.mac
```

## Source and phantom geometry

The default geometry is a 30 cm cubic `G4_AIR` world (density 0.00120479 g/cm3) containing three concentric, centered regions: a 5 cm tall metallic Cs-137 cylinder (radius 2.375 mm, density 1.873 g/cm3), an open-ended `G4_STAINLESS-STEEL` cylindrical wall (density 8 g/cm3, 0.8 mm radial thickness), and a `G4_WATER` annular phantom (density 1 g/cm3, outer radius 33.210 mm, height 5 cm). The Cs-137 material uses a single Cs-137 isotope. All three regions have the same height.

The dimensions can be set before `/run/initialize` through `/sourceLab/geometry/*`:

```tcl
/sourceLab/geometry/worldSize 30 cm
/sourceLab/geometry/phantomHeight 5 cm
/sourceLab/geometry/sourceRadius 2.375 mm
/sourceLab/geometry/sourceShellThickness 0.8 mm
```

Dose, energy deposit, and charged track length are scored in five contiguous water annuli. Regions 0, 2, and 4 are 0.07 mm thick: region 0 starts immediately outside the steel, and regions 2 and 4 are centered 10 mm and 30 mm outside the steel, respectively. Regions 1 and 3 fill the gaps. All have the phantom height. Region 4 ends at the phantom's outer radius and borders the air world. The radius is derived from the source radius and wall thickness; `/sourceLab/geometry/phantomRadius` has been removed. Initialization rejects a phantom that does not fit inside the world.

The source, shell, and water annuli are separate sibling volumes in the air world. Each primitive dose scorer uses its annulus's water mass. The existing `phantom` event ntuple and `run` summary contain whole-water scores, with dose calculated as total water energy deposit divided by total water mass. At run end, the mass-weighted sum of the five region doses and the accumulated whole-water dose are checked against total water energy divided by total mass (relative tolerance `1e-10`, with exact agreement for zero); a mismatch is fatal. Both calculations use the same region hits, so this checks scoring normalization and aggregation rather than independently validating the geometry or physics.

The `water_regions` ntuple contains five rows per event, with `EventId`, `ThreadId`, `Region` (copy numbers 0–4), `Dose`, `Edep`, and `TrackL`. The `water_region_run` ntuple has five rows per run in sequential mode or per worker in MT mode, with `ThreadId`, `Region`, `InnerRadiusMm`, `OuterRadiusMm`, `MassG`, `Dose`, `Edep`, and `TrackL`. Dose, energy, and track-length columns use Geant4 internal units, as in the existing ntuples. Sum each region's run scores across workers; region masses and radii are repeated, not additive. `/sourceLab/output/depth` remains metadata and defaults to `0 cm`.

The default runs and `run-decay-cs137.mac` sample Cs-137 ions uniformly throughout the inner source cylinder. The GPS radius and half-height in `position-cs137-cylinder.mac` match the default geometry dimensions. If the source radius or phantom height is changed, update the GPS radius or half-height to match.

## Output metadata and file naming

sourceLab exposes output metadata commands under `/sourceLab/output/`.

Set metadata in a macro before `/run/beamOn`:

```tcl
/sourceLab/output/tag co60-cylinder
/sourceLab/output/source co60
/sourceLab/output/geometry water-annulus
/sourceLab/output/region phantom
/sourceLab/output/depth 0 cm
```

Allowed source labels are: `unspecified`, `co60`, `cs137`, `6mv`, `10mv`.
Legacy aliases like `linac-6mv`, `linac-10mv`, `cs-137`, and `cobalt-60` are accepted and canonicalized.
The MV labels are retained for metadata compatibility; sourceLab does not provide MV source preset macros.

Output file naming:

- default tag (unset): `sourceLab-default.root`
- custom tag: `sourceLab-<tag>.root`

Allowed `tag` characters are letters, digits, `-`, and `_`.

The `runinfo` ntuple uses the standardized schema:

- `Tag`
- `Source`
- `Geometry`
- `Region`
- `DepthCm`
- `EMModel`
- `RadioactiveDecay`
- `Events`
- `ThreadId`
- `Geant4Version` (version used to build the application)
- `RandomEngine` (engine used by the scoring thread)

Policy: `runinfo` stores one row per run in sequential mode, and one row per worker in MT mode. Use `ThreadId` to identify worker provenance.

Each `score_statistics` row describes one quantity (`Dose`, `Edep`, or charged `TrackL`) per worker, or the whole run in sequential mode. `Region` is -1 for whole water and 0–4 for the five annuli; columns are `ThreadId`, `Region`, `Quantity`, `Events`, `Mean`, `M2`, `StdDev`, and `StdErr`. Every event is included, including zero scores. `M2` is the sum of squared deviations from the mean; `StdDev = sqrt(M2/(Events-1))` is the sample standard deviation and `StdErr = StdDev/sqrt(Events)` estimates the uncertainty of the per-event mean for independent events. For fewer than two events, `StdDev` and `StdErr` are NaN. Scores use Geant4 internal units; `M2` uses their squares.

For MT output, do not average worker standard deviations or errors. For each region and quantity, combine worker counts, means, and M2 using `N = sum(n_i)`, `mean = sum(n_i*mean_i)/N`, and `M2 = sum(M2_i + n_i*(mean_i-mean)^2)`, ignoring empty workers. Then calculate the run-level standard deviation and standard error as above. The console also prints the combined statistics. Whole-water event scores are summed before computing statistics, preserving correlations between regions. These are statistical uncertainties only, not systematic uncertainties or validation of the physics model. Seed selection and provenance are unchanged and are not recorded yet.

At end-of-run, each worker writes its data for merging, and the master finalizes the output. A concise "Run complete" summary line is printed with events, output file, and key metadata.

When radioactive decay is enabled (`-r on`), sourceLab globally applies a long-decay-time threshold (`/process/had/rdm/thresholdForVeryLongDecayTime 1e+60 year`) at run start so long-lived ions (for example Cs-137) can decay within simulated events.

## Harmonized macro lifecycle and ownership

The three labs follow a shared macro lifecycle even though each one emphasizes a slightly different startup style.

1. Physical geometry setup comes first. Raw detector dimensions, materials, shell thickness, cut values, and axis/orientation are detector-owned state and belong in a dedicated detector messenger / construction layer.
2. Scenario and placement setup comes next. depth, position, or preset commands are orchestration-level steps: they determine how the detector sits in context and keep output metadata consistent with the active run.
3. Source setup follows. Source preset macros define only the particle identity and emission spectrum; they do not initialize the run or fire beamOn.
4. Field setup follows. Field macros define beam size, direction, SSD, and the geometry of the irradiation pattern.
5. Initialization happens once a complete scenario is assembled. `/run/initialize` freezes the geometry and source/field configuration for the run.
6. Visualization, scoring, and execution happen after initialization. Viewer commands, scorers, and `/run/beamOn` are run only after the detector and source state have been established.

This gives us the best of both patterns: doseLab keeps its modular scenario-first structure, while the newer labs remain simpler and easier to inspect in visual mode. The key rule is that raw geometry remains detector-owned, scenario/output metadata remains orchestration-owned, and source/field/scoring/vis remain native Geant4 subsystems.

## Cross-lab macro contract

The three labs follow one shared macro structure contract so geometry differences do not fragment workflow conventions.

CLI contract (all labs):

- `-b <macro>` batch mode
- `-v <macro>` visual mode
- `-t <N>` threads (MT builds)
- `-p option4|livermore|penelope` EM model selection
- `-r on|off` radioactive decay physics toggle

Namespace contract:

- geometry commands stay under a lab prefix, for example `/sourceLab/geometry/*`
- output metadata commands stay under `/.../output/*`
- scenario/preset commands (when present) stay under `/.../scenario/*`

Source composition contract:

- run macros compose source via `/control/execute source-<preset>.mac`
- source preset macros set source identity and spectrum only
- source preset macros must not call `/run/initialize` or `/run/beamOn`
- optional compatibility alias macro is `source.mac`

Available source presets:

- `source-co60.mac`
- `source-cs137.mac` (default; Cs-137 decays via Ba-137m)
- `source.mac` (alias for `source-cs137.mac`)

Small decay example run macro:

- `run-decay-cs137.mac`
- run with: `./scripts/run-micromamba.sh run -b run-decay-cs137.mac`

Nested macro path robustness:

- add `/control/macroPath .:macros:../macros` before nested `/control/execute` in composed run macros
- this keeps behavior consistent for repo-root and build-directory invocation styles

## SourceLab slab contract (pre-implementation)

This section defines the slab preset and override semantics before slab geometry code is added. The goal is deterministic behavior and stable cross-lab conventions.

Planned command namespace:

- `/sourceLab/scenario/type <preset>`
- `/sourceLab/scenario/reset`
- `/sourceLab/slabs/material <nistName>`
- `/sourceLab/slabs/thickness <value> <unit>`
- `/sourceLab/slabs/gap <value> <unit>`
- `/sourceLab/slabs/offset <value> <unit>`
- `/sourceLab/slabs/mode fixed|approach`
- `/sourceLab/slabs/step <value> <unit>`
- `/sourceLab/slabs/minGap <value> <unit>`
- `/sourceLab/slabs/print`

Planned presets:

- `open`: slabs far from source, no enclosure
- `approach`: symmetric closing motion toward source
- `enclosed`: source enclosed by slab faces
- `custom`: no preset defaults, explicit values only

Override and precedence rules:

1. Applying `/sourceLab/scenario/type <preset>` sets all preset-owned slab parameters.
2. Explicit `/sourceLab/slabs/*` commands then override individual parameters.
3. Re-applying `/sourceLab/scenario/type <preset>` re-applies preset defaults and overwrites prior slab overrides.
4. Last command wins before `/run/initialize`.
5. Geometry is frozen after `/run/initialize` for the active run.

State contract:

| Command group | Allowed states | Notes |
| --- | --- | --- |
| `/sourceLab/scenario/*` | `PreInit`, `Idle` | Preset selection and reset only. |
| `/sourceLab/slabs/*` | `PreInit`, `Idle` | Parameter overrides and inspection. |
| `/run/initialize` | Geant4 standard | Freezes effective slab configuration for run. |
| `/run/beamOn` | Geant4 standard | No geometry mutation during beamOn. |

Run metadata contract (runinfo):

- `Scenario`: selected preset name (`open`, `approach`, `enclosed`, `custom`)
- `SlabMaterial`
- `SlabThicknessMm`
- `SlabGapMm`
- `SlabOffsetMm`
- `SlabMode`
- `SlabStepMm`
- `SlabMinGapMm`

These are in addition to the shared runinfo fields already used across labs (`Tag`, `Source`, `Geometry`, `Region`, `DepthCm`, `EMModel`, `RadioactiveDecay`, `Events`, `ThreadId`, `Geant4Version`, `RandomEngine`).

## AI Usage

AI-assisted development is used in this project.
All commits are reviewed by humans.

## Acknowledgements

- This repository is released under the MIT License. See LICENSE.
- This project uses Geant4 and follows the Geant4 license terms: http://cern.ch/geant4/license
- Parts of this code and documentation workflow were developed with assistance from GitHub Copilot in VS Code.
