# Explicit Phoebus screen bindings

`Source_Stability_Observer.bob` now uses explicit `SSO:` names. There is no PV
prefix macro involved in any widget connection.

Every dynamic textual readback is a `text_symbol` widget with its Symbols list
set to `$(pv_value)`. This matches the target Phoebus installation and avoids the
unsupported `text_update` widgets that produced model-loading errors.

## Status band

| Widget | PV |
|---|---|
| Simulation LED | `SSO:SIM:ACTIVE` |
| Injected scenario | `SSO:SIM:FAULT_MODE` |
| Acquisition LED | `SSO:T8:CONNECTED` |
| Sample rate | `SSO:T8:SAMPLE_RATE` |
| Spot-valid LED | `SSO:SPOT:VALID` |
| Fit quality | `SSO:SPOT:FIT_QUALITY` |
| Instability LED | `SSO:SPOT:UNSTABLE` |
| Primary hypothesis | `SSO:OBS:PRIMARY_HYPOTHESIS` |
| IOC heartbeat/time | `SSO:IOC:HEARTBEAT`, `SSO:IOC:TIME_OF_DAY` |

## Image and spot metrics

`SSO:SPOT:IMAGE`, `SSO:SPOT:FWHM_X`, `SSO:SPOT:FWHM_Y`,
`SSO:SPOT:CENTROID_X`, `SSO:SPOT:CENTROID_Y`, `SSO:SPOT:PEAK`, and
`SSO:SRC:EXPOSURE_ID`.

## Source inputs

`SSO:T8:KV:MEAS`, `SSO:T8:TUBE_CURRENT`, `SSO:T8:RETURN_CURRENT`,
`SSO:T8:TEMPERATURE`, `SSO:T8:VACUUM`, and `SSO:T8:XRAY_FLUX`.

## C observer outputs

`SSO:OBS:CORR:LEAKAGE`, `SSO:OBS:CORR:TUBE_CURRENT`,
`SSO:OBS:CORR:KV_ERROR`, `SSO:OBS:CORR:TEMPERATURE`,
`SSO:OBS:SPOT_CV`, and `SSO:OBS:STATUS_TEXT`.

## Live synchronized trend

The Strip Chart uses `SSO:SPOT:FWHM_X`, `SSO:SPOT:FWHM_Y`,
`SSO:T8:KV:MEAS`, `SSO:T8:TUBE_CURRENT`, `SSO:T8:RETURN_CURRENT`,
`SSO:T8:TEMPERATURE`, `SSO:T8:VACUUM`, and `SSO:T8:XRAY_FLUX`.

Select an individual Text Symbol and inspect its **PV Name** property to see the
explicit binding. Its Symbol property is `$(pv_value)`.

With the IOC running, verify every page binding from the project root:

```sh
./phoebus/verify_display_pvs.sh
```
