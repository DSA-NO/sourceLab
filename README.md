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

## AI Usage

AI-assisted development is used in this project.
All commits are reviewed by humans.
