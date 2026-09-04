# sourceLab

This project is a Geant4 application focused on simulating radioactive and source-driven particle emission using a GPS-based primary generator setup.

## Quick start

Use the shipped micromamba helper for the recommended workflow:

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
