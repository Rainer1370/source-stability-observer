#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scenario="${1:-leakage}"
run_dir="$project_root/run"
ioc_log="$run_dir/softioc.log"
ioc_pid=""

case "$scenario" in
    normal|leakage|emission|kv-ripple|thermal|cable|optical|bad-image) ;;
    *)
        printf 'Unknown scenario: %s\n' "$scenario" >&2
        printf 'Choose: normal leakage emission kv-ripple thermal cable optical bad-image\n' >&2
        exit 2
        ;;
esac

cleanup() {
    if [[ -n "$ioc_pid" ]] && kill -0 "$ioc_pid" 2>/dev/null; then
        kill "$ioc_pid" 2>/dev/null || true
        wait "$ioc_pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

"$project_root/scripts/check_dependencies.sh"
if [[ ! -x "$project_root/.venv/bin/python" ]]; then
    printf 'Python environment is missing. Run scripts/setup_ubuntu.sh first.\n' >&2
    exit 1
fi
if ! "$project_root/.venv/bin/python" -c 'import epics, numpy' >/dev/null 2>&1; then
    printf 'Python packages are missing from .venv. Run scripts/setup_ubuntu.sh first.\n' >&2
    exit 1
fi

make -C "$project_root" test libsource_observer.so
mkdir -p "$run_dir"

printf 'Starting EPICS soft IOC...\n'
"$project_root/ioc/run_softioc.sh" >"$ioc_log" 2>&1 &
ioc_pid=$!

ready=0
for _ in {1..40}; do
    if ! kill -0 "$ioc_pid" 2>/dev/null; then
        printf 'softIoc exited during startup. Log follows:\n' >&2
        sed -n '1,160p' "$ioc_log" >&2
        exit 1
    fi
    if command -v caget >/dev/null 2>&1; then
        if caget -t SSO:IOC:HEARTBEAT >/dev/null 2>&1; then
            ready=1
            break
        fi
    else
        sleep 0.1
        ready=1
        break
    fi
    sleep 0.1
done
if (( ! ready )); then
    printf 'IOC did not answer SSO:IOC:HEARTBEAT. See %s\n' "$ioc_log" >&2
    exit 1
fi

printf 'IOC ready (PID %s). Running scenario: %s\n' "$ioc_pid" "$scenario"
printf 'Open Phoebus display: %s/phoebus/Source_Stability_Observer.bob\n' "$project_root"

if [[ "${START_PHOEBUS:-0}" == "1" ]]; then
    phoebus_cmd="${PHOEBUS_CMD:-/opt/phoebus/phoebus-product/phoebus.sh}"
    if [[ -x "$phoebus_cmd" ]]; then
        "$phoebus_cmd" -resource "$project_root/phoebus/Source_Stability_Observer.bob" \
            >"$run_dir/phoebus.log" 2>&1 &
    else
        printf 'Phoebus requested but launcher is not executable: %s\n' "$phoebus_cmd" >&2
    fi
fi

"$project_root/.venv/bin/python" "$project_root/python/simulate_epics.py" \
    --scenario "$scenario"
