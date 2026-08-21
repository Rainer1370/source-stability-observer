#!/usr/bin/env bash
set -euo pipefail

if ! command -v caget >/dev/null 2>&1; then
    printf 'caget is required for this smoke test.\n' >&2
    exit 1
fi

printf 'IOC heartbeat:      %s\n' "$(caget -t SSO:IOC:HEARTBEAT)"
printf 'Simulation active:  %s\n' "$(caget -t SSO:SIM:ACTIVE)"
printf 'T8 sample rate:     %s Hz\n' "$(caget -t SSO:T8:SAMPLE_RATE)"
printf 'Spot FWHM X:        %s um\n' "$(caget -t SSO:SPOT:FWHM_X)"
printf 'Observer ready:     %s\n' "$(caget -t SSO:OBS:READY)"
printf 'Observer status:    %s\n' "$(caget -t SSO:OBS:STATUS_TEXT)"
printf 'Accepted/rejected:  %s / %s\n' \
    "$(caget -t SSO:OBS:ACCEPTED_COUNT)" "$(caget -t SSO:OBS:REJECTED_COUNT)"
