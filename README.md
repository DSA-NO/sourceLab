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

The default run macro now uses composed source presets:

- `macros/source-co60.mac`: canonical Co-60 source preset
- `macros/source.mac`: compatibility alias that executes `source-co60.mac`

For a quick visualization run:

```bash
./scripts/run-micromamba.sh run -v init_vis.mac
```

Use `-p option4|livermore|penelope` to select the EM sub-model. For example:

```bash
./scripts/run-micromamba.sh run -p livermore -b run.mac
```

Use `-r on|off` to enable or disable radioactive decay physics. For example:

```bash
./scripts/run-micromamba.sh run -r on -b run.mac
```

## Output metadata and file naming

sourceLab exposes output metadata commands under `/sourceLab/output/`.

Set metadata in a macro before `/run/beamOn`:

```tcl
/sourceLab/output/tag co60-depth5
/sourceLab/output/source co60
/sourceLab/output/geometry waterbox
/sourceLab/output/region sample
/sourceLab/output/depth 5 cm
```

Allowed source labels are: `unspecified`, `co60`, `cs137`, `6mv`, `10mv`.
Legacy aliases like `linac-6mv`, `linac-10mv`, `cs-137`, and `cobalt-60` are accepted and canonicalized.

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

Policy: `runinfo` stores one row per run in sequential mode, and one row per worker in MT mode. Use `ThreadId` to identify worker provenance.

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

Shared source preset catalog (required naming):

- `source-co60.mac`
- `source-6mv.mac`
- `source-10mv.mac`
- `source.mac` (alias macro pointing to the default preset for quick runs)

Decay-enabled example preset:

- `source-cs137.mac` (run with `-r on`; Cs-137 decays via Ba-137m)

Small decay example run macro:

- `run-decay-cs137.mac`
- run with: `./scripts/run-micromamba.sh run -r on -b run-decay-cs137.mac`

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

These are in addition to the shared runinfo fields already used across labs (`Tag`, `Source`, `Geometry`, `Region`, `DepthCm`, `EMModel`, `RadioactiveDecay`, `Events`, `ThreadId`).

## AI Usage

AI-assisted development is used in this project.
All commits are reviewed by humans.
