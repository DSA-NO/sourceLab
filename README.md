# sourceLab

This project is a Geant4 application focused on simulating radioactive and source-driven particle emission using a GPS-based primary generator setup.

## Quick start

If micromamba is missing or stale, refresh it first. These helper scripts are intended for Linux/macOS shells; on Windows, use WSL, Git Bash, or another Unix-like shell and follow the same steps.

```bash
cd sourceLab
./scripts/install-micromamba.sh
export PATH="$HOME/.local/bin:$PATH"
```

Then use the shipped micromamba helper for the recommended workflow:

```bash
cd sourceLab
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

## Fresh-user clone test (recommended)

This is a good idea for reproducibility checks. It simulates a first-time user in a clean shell, clones from GitHub, and runs the same helper workflow.

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
