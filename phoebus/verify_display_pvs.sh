#!/usr/bin/env bash
set -u

display="${1:-$(dirname "$0")/Source_Stability_Observer.bob}"
if ! command -v caget >/dev/null 2>&1; then
    echo "ERROR: caget is not on PATH" >&2
    exit 2
fi

mapfile -t pvs < <(
    sed -n \
        -e 's:.*<pv_name>\([^<]*\)</pv_name>.*:\1:p' \
        -e 's:.*<y_pv>\([^<]*\)</y_pv>.*:\1:p' \
        "$display" | sort -u
)
if ((${#pvs[@]} == 0)); then
    echo "ERROR: no pv_name or y_pv elements found in $display" >&2
    exit 2
fi

failed=0
printf 'Checking %d explicit screen PVs from %s\n' "${#pvs[@]}" "$display"
for pv in "${pvs[@]}"; do
    if value=$(caget -t -w 1 "$pv" 2>/dev/null); then
        printf 'OK      %-38s %s\n' "$pv" "$value"
    else
        printf 'MISSING %s\n' "$pv"
        failed=$((failed + 1))
    fi
done

printf '\nResult: %d connected, %d missing\n' "$((${#pvs[@]} - failed))" "$failed"
exit "$failed"
