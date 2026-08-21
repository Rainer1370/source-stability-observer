# Alarm philosophy for the simulation IOC

The IOC uses EPICS record alarms to demonstrate operational behavior, not to
declare field or product safety limits. Every numeric limit is a startup
macro in `iocBoot/iocSourceObserver/st.cmd`.

## Severity convention

- `NO_ALARM`: expected simulated operation.
- `MINOR`: degraded or approaching an illustrative diagnostic boundary.
- `MAJOR`: invalid diagnostic input, disconnected acquisition, unstable spot,
  or a simulated value well outside the demonstration envelope.

Record limit alarms (`HIHI/HIGH/LOW/LOLO`) use `HYST` where chatter is likely.
Binary records use `ZSV` or `OSV`: disconnected T8, invalid spot, bad observer
data, and unstable spot are major; an event pulse is minor.

## Deliberate separations

1. A process alarm says a value crossed a configured boundary.
2. `SPOT:UNSTABLE` says the C observer found statistical instability.
3. `OBS:*ASSOCIATED*` or correlation PVs say domains co-varied.
4. None of these alone establishes root cause.

The real system would derive limits from the source, supply, transducer, and
operating procedure specifications and would keep personnel/equipment safety in
independent rated interlocks—not in this soft IOC.
