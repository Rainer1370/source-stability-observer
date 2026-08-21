# System and LabJack T8 engineering overview

## The problem statement

This demonstrator considers inconsistent X-ray spot size without presuming whether
the cause is the electron/source subsystem, high-voltage behavior, leakage/ground
return, cabling, thermal drift, mechanics/optics, or the spot measurement itself.
The external observer is designed to discriminate among those hypotheses.

Be exact about *which* spot is measured. Electron focal spot, emitted X-ray source
spot, and focused spot at the sample are different system quantities. This demo calls
the image-derived values `spot_sigma_x/y`; the real instrument must document where
and how that image is formed.

## Measurement architecture

| T8 input | Low-voltage signal | Required front end | Diagnostic use |
|---|---|---|---|
| AIN0 | HV monitor | Vendor monitor output or rated isolated divider | kV error/ripple |
| AIN1 | Tube/emission current monitor | Vendor monitor or isolated transducer | emission stability |
| AIN2 | Return/leakage proxy | Certified current transducer + burden/conditioner | unintended return association |
| AIN3 | Filament current proxy | Hall/RMS transducer | cathode/heater behavior |
| AIN4 | Source temperature | conditioned RTD/thermocouple transmitter | thermal association |
| AIN5 | Vacuum gauge output | gauge controller's isolated analog output | vacuum events |
| AIN6 | X-ray flux monitor | diode/scintillator amplifier output | dose stability |
| AIN7 | external trigger | conditioned pulse/level, or use digital I/O | exposure alignment |

The T8 measures only signals within its input limits. It is **not** connected directly
to a high-voltage node. Sensor isolation, working voltage, creepage/clearance,
bandwidth, loading, fusing, and facility X-ray/HV procedures require qualified review.

Image spot metrics usually arrive over a separate camera/detector path. Join them by
monotonic time and exposure ID; measure clock offset and jitter. A simultaneous ADC
cannot make an asynchronously acquired image simultaneous by itself.

## T8 facts relevant to the design

Current official documentation describes eight individually isolated, simultaneously
sampled, 24-bit analog inputs, programmable ranges from ±11 V down to ±0.018 V,
and 1000 V isolation. LabJack documents up to 40 ksample/s/channel (400 ksample/s
aggregate in the current analog-input page), plus 20 digital I/O, Ethernet/USB,
hardware timing/counters, and LJM support on Linux.

Why it fits: simultaneous isolated inputs reduce inter-channel skew and accidental
ground-loop paths when correlating transients. Why it is not magic: transducers set
the real bandwidth, safety rating, noise, and accuracy; eight channels may be too few;
and isolation does not authorize direct HV probing.

Two acquisition modes matter:

- Command/response: one AIN read triggers simultaneous conversion; other channels can
  be read from `_CAPTURE` registers. Good for slow snapshots.
- Stream: configure channel addresses, call `LJM_eStreamStart`, repeatedly call
  `LJM_eStreamRead`, and monitor device/library backlog. Good for continuous waveforms.

Official references:

- [T8 product page](https://labjack.com/products/t8)
- [T8 analog inputs](https://support.labjack.com/docs/14-3-2-analog-inputs-t8-t-series-datasheet)
- [LJM overview](https://support.labjack.com/docs/ljm-library-overview)
- [LJM C/C++ examples](https://support.labjack.com/docs/c-c-for-ljm-windows-mac-linux)

## Physics guardrails

At the supply boundary, a measured current can include useful tube current, leakage,
and displacement current: `I_supply = I_tube + I_leak + C*dV/dt`. Therefore the code
must label ramp/settle/exposure phases. A return residual during ramp is not proof of
leakage. Correlation during steady operation is a test lead, not a root-cause verdict.

## Simulated functions

| Scenario | Injected behavior | Expected interpretation |
|---|---|---|
| normal | sensor noise only | no spot alarm |
| leakage | return residual and spot co-vary | inspect grounding/cable/insulation path |
| emission | tube current, flux, and spot oscillate | inspect cathode/filament/regulation |
| kv-ripple | measured kV error and spot co-vary | inspect supply/load/feedback behavior |
| thermal | temperature and spot drift together | thermal stabilization/alignment test |
| cable | periodic discontinuity across domains | connector/cable stress test |
| optical | spot/centroid changes without electrical match | optics/mechanics or estimator branch |

All numerical values are illustrative and configurable; they are not specifications
for any commercial product.

## Verification plan before hardware

1. Fixed-seed tests prove repeatability and reject a normal false alarm.
2. Each injected fault must activate its intended diagnostic branch.
3. CSV plots verify time alignment and reveal threshold edge cases.
4. Replay recorded benign signals before any live integration.
5. With a T8, inject traceable low-voltage calibrator signals; verify scale, polarity,
   clipping, channel mapping, timestamp gaps, and backlog handling.
6. Only then connect approved conditioned outputs from an interlocked system.
