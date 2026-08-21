#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
epics_base="${EPICS_BASE:-/usr/lib/epics}"
host_arch="${EPICS_HOST_ARCH:-linux-x86_64}"
run_after_build=0

usage() {
    cat <<'EOF'
Usage: ./scripts/build_native_ioc.sh [--run]

Build the conventional EPICS IOC application. With --run, start the IOC with
ioc/iocBoot/iocSourceObserver/st.cmd and remain in the interactive IOC shell.

Environment overrides:
  EPICS_BASE       EPICS Base installation (default: /usr/lib/epics)
  EPICS_HOST_ARCH  EPICS host architecture (default: linux-x86_64)
EOF
}

case "${1:-}" in
    "") ;;
    --run) run_after_build=1 ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
esac

if [[ ! -f "$epics_base/configure/CONFIG" ]]; then
    printf 'EPICS Base build files not found at %s\n' "$epics_base" >&2
    printf 'Set EPICS_BASE to the correct installation and retry.\n' >&2
    exit 1
fi

printf 'Building native IOC with EPICS_BASE=%s\n' "$epics_base"
make -C "$project_root/ioc" clean
make -C "$project_root/ioc"

ioc_binary="$project_root/ioc/bin/$host_arch/sourceObserver"
startup_dir="$project_root/ioc/iocBoot/iocSourceObserver"

if [[ ! -x "$ioc_binary" ]]; then
    printf 'Expected IOC executable was not produced: %s\n' "$ioc_binary" >&2
    exit 1
fi

printf 'Native IOC build completed: %s\n' "$ioc_binary"

if (( run_after_build )); then
    cd "$startup_dir"
    exec "$ioc_binary" st.cmd
fi
