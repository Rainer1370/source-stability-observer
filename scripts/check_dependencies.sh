#!/usr/bin/env bash
set -u

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
failures=0
warnings=0

ok()   { printf '  [OK]   %s\n' "$*"; }
fail() { printf '  [MISS] %s\n' "$*"; failures=$((failures + 1)); }
warn() { printf '  [WARN] %s\n' "$*"; warnings=$((warnings + 1)); }

have_command() {
    command -v "$1" >/dev/null 2>&1
}

printf 'Source Stability Observer dependency check\n'
printf 'Project: %s\n\n' "$project_root"

printf 'C toolchain\n'
for command_name in cc make; do
    if have_command "$command_name"; then
        ok "$command_name: $(command -v "$command_name")"
    else
        fail "$command_name (Ubuntu package: build-essential)"
    fi
done

printf '\nPython\n'
if have_command python3; then
    ok "python3: $(python3 --version 2>&1)"
    if python3 -m venv --help >/dev/null 2>&1; then
        ok "venv module available"
    else
        fail "python3 venv module (Ubuntu package: python3-venv)"
    fi
    if python3 -m pip --version >/dev/null 2>&1; then
        ok "pip module available"
    else
        fail "python3 pip module (Ubuntu package: python3-pip)"
    fi
else
    fail "python3"
fi

printf '\nEPICS Base / Channel Access\n'
if have_command softIoc; then
    ok "softIoc: $(command -v softIoc)"
else
    fail "softIoc (install EPICS Base or export its bin directory in PATH)"
fi
for command_name in caget caput camonitor; do
    if have_command "$command_name"; then
        ok "$command_name: $(command -v "$command_name")"
    else
        warn "$command_name unavailable; useful for smoke tests"
    fi
done

if [[ -n "${EPICS_BASE:-}" ]]; then
    if [[ -d "$EPICS_BASE" ]]; then
        ok "EPICS_BASE=$EPICS_BASE"
    else
        warn "EPICS_BASE is set but is not a directory: $EPICS_BASE"
    fi
else
    warn "EPICS_BASE is not set; database-only softIoc launch can still work"
fi

printf '\nOptional interfaces\n'
if ldconfig -p 2>/dev/null | grep -q 'libLabJackM'; then
    ok "LabJack LJM runtime library detected"
else
    warn "LabJack LJM not detected; simulation works, physical T8 acquisition does not"
fi
if [[ -x /opt/phoebus/phoebus-product/phoebus.sh ]] || have_command phoebus; then
    ok "Phoebus launcher detected"
else
    warn "Phoebus launcher not detected; pass PHOEBUS_CMD when installed elsewhere"
fi

printf '\nResult: %d missing requirement(s), %d warning(s).\n' "$failures" "$warnings"
if (( failures > 0 )); then
    printf 'Run: scripts/setup_ubuntu.sh --install\n'
    exit 1
fi
printf 'Required dependencies are present.\n'
