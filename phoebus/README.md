# Phoebus Source Stability Observer display

Open `Source_Stability_Observer.bob` in Phoebus Display Builder. Every widget is
bound to an explicit `SSO:` PV; the display does not depend on a prefix macro.

All dynamic textual readbacks use the Phoebus **Text Symbol** widget required by
the target installation. Each has one symbol, `$(pv_value)`, so it displays the
live value of its explicitly assigned PV. LEDs, the Intensity Graph, and the
Strip Chart retain their purpose-built widget types.

The upper-left Intensity Graph consumes the flattened 128x128 float waveform
`SSO:SPOT:IMAGE`. The values beside it are the fitted spot metrics. The upper-right
panel separates raw source measurements from the C observer's correlations. The
bottom Strip Chart places eight live traces on the same ten-minute timebase, with
separate Y axes because their engineering units differ.

The Strip Chart is a live synchronized view. For archive retrieval, right-click a
PV and add it to the Phoebus Data Browser, or create a `.plt` after the Archiver
Appliance URL is configured for the target installation. The scalar PV list in
`../ioc/archive_pvs.txt` is the proposed archive configuration; the image waveform
is intentionally excluded.

## Expected behavior before the C observer is connected

The Python digital twin will animate the image and raw values. `OBS:*` values and
`SPOT:UNSTABLE` will remain at their defaults until the C observer publishes them.

## Compatibility note

Phoebus plot serialization has changed slightly among releases. This display uses
current compact `.bob` widget conventions. If an installed release rewrites the
Intensity Graph or Strip Chart when first saved, allow the editor to save its
local canonical form; PV names and layout remain unchanged.
