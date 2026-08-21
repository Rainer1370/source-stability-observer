# Source Observer EPICS soft IOC

This is a conventional EPICS Base IOC. It exposes a stable PV contract for the
digital twin, the future LabJack T8 adapter, the C observer, Phoebus, and an
EPICS Archiver. All process limits are illustrative simulation defaults.

## Build and run

Edit `configure/RELEASE` if EPICS Base is not `/usr/lib/epics`, then:

```sh
cd ioc
make
cd iocBoot/iocSourceObserver
chmod +x st.cmd
./st.cmd
```

In a second terminal, from the project root:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r python/requirements.txt
python python/simulate_epics.py --scenario leakage
```

Use `caget SSO:IOC:HEARTBEAT` and `camonitor SSO:SPOT:UNSTABLE` for a smoke test.

For a faster database-only launch on a machine that already has `softIoc`:

```sh
cd ioc
./run_softioc.sh
```

`SSO:SPOT:IMAGE` is a flat 128x128 Channel Access waveform accompanied by width
and height PVs. Display it with a Phoebus Intensity Graph. A later areaDetector
or normative-type bridge can publish NTNDArray for the dedicated Image widget.

The simulator owns `SSO:SIM:*`, `SSO:T8:*`, `SSO:SRC:*`, and `SSO:SPOT:*`.
The future C observer owns `SSO:OBS:*` and `SSO:SPOT:UNSTABLE`. The observer must
never read `SSO:SIM:FAULT_MODE`; that PV is verification ground truth only.
