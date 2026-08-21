# Installation and operation on Ubuntu

## First local deployment

```bash
git clone https://github.com/Rainer1370/source-stability-observer.git
cd source-stability-observer

./scripts/check_dependencies.sh
./scripts/setup_ubuntu.sh --install
./scripts/start_demo.sh leakage
```

The package installation flag installs only prerequisites available from the
enabled Ubuntu repositories. If `epics-base` is unavailable there, install EPICS
Base separately and add its `bin/<architecture>` directory to `PATH` before
running the checker again.

The setup script creates `.venv`, installs `numpy` and `pyepics`, builds all C
targets, and executes the test suite. The start script launches the soft IOC,
waits for its heartbeat, runs the selected simulator scenario, and terminates
the IOC when the simulator is stopped.

## Subsequent use

```bash
git clone https://github.com/Rainer1370/source-stability-observer.git
cd source-stability-observer
./scripts/setup_ubuntu.sh
./scripts/start_demo.sh leakage
```

Use `--install` only when the dependency check reports missing Ubuntu packages.

## Useful checks

```bash
make test
./scripts/smoke_test.sh
caget SSO:OBS:STATUS_TEXT
camonitor SSO:SPOT:UNSTABLE SSO:OBS:PRIMARY_HYPOTHESIS
```
