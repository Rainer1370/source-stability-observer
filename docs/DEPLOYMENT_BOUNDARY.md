# Production-readiness boundary

The implementation separates the diagnostic path into the following components:

- **C observer:** accepts only exposure-aligned, quality-gated records; computes spot CV and numerically stable Pearson associations; reports associations, not causes.
- **C exposure synchronizer:** retains a bounded ring of timestamped T8 scans, selects scans inside each detector exposure, checks coverage and gaps, averages the external diagnostics, and propagates the exposure ID.
- **C LJM acquisition adapter:** owns the LabJack handle and stream lifecycle, validates buffers and state, and exposes interleaved AIN0–AIN7 scans. It is compiled against LabJack LJM only when `HAVE_LJM` is defined.
- **Python simulator:** generates repeatable verification stimulus and publishes PVs. It is not the intended production acquisition or analysis engine.

## Data path

1. The Linux acquisition coordinator creates an exposure ID and records the request timestamp.
2. The detector client issues the software Acquire command and later returns the frame, completion timestamp, fit metrics, and the same pending exposure ID.
3. Independently, the LJM worker drains the T8 hardware-timed Ethernet stream into timestamped scan records.
4. The C synchronizer selects the T8 records between request and completion. Too few scans, an excessive timestamp gap, invalid timing, or a non-finite value marks the aligned record invalid.
5. Only a valid aligned record enters the observer's rolling window. A poor spot fit or invalid spot metric is also rejected before it can influence CV or correlation.

## Still required before deployment on physical equipment

- Verify the target equipment's monitor outputs, ranges, isolation, scaling, grounding, and calibration coefficients.
- Configure T8 ranges, resolution index, settling time, negative channels, and stream clock from measured bandwidth requirements.
- Implement the EPICS `asynPortDriver` integration and its worker thread around this C adapter (the EPICS class itself is C++).
- Integrate the real detector IOC/client and validate exposure-ID retention across timeouts and retries.
- Establish a common Linux timebase and measure detector-command, completion, network, and T8 timestamp uncertainty.
- Add disconnect/reconnect, backlog, overrun, stale-data, and orderly-shutdown state-machine tests against a physical T8.
- Run long-duration soak, fault-injection, calibration, and safety reviews. This observer must remain read-only and outside the HV protection chain.

The demonstration therefore shows a credible, tested diagnostic architecture.
A simulated correlation does not establish root cause, and the package is not yet
qualified for machine operation.
