#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
install_requested=0
if [[ "${1:-}" == "--install" ]]; then
    install_requested=1
elif [[ $# -gt 0 ]]; then
    printf 'Usage: %s [--install]\n' "$0" >&2
    exit 2
fi

if (( install_requested )); then
    if ! command -v apt-get >/dev/null 2>&1; then
        printf 'This installer supports Ubuntu/Debian apt systems only.\n' >&2
        exit 1
    fi
    packages=(build-essential python3 python3-venv python3-pip)
    if apt-cache show epics-base >/dev/null 2>&1; then
        packages+=(epics-base)
    else
        printf 'NOTE: epics-base is not available in the enabled apt repositories.\n'
        printf 'Install EPICS Base separately, then add its bin/<arch> directory to PATH.\n'
    fi
    printf 'Installing: %s\n' "${packages[*]}"
    sudo apt-get update
    sudo apt-get install -y "${packages[@]}"
fi

"$project_root/scripts/check_dependencies.sh"

if [[ ! -d "$project_root/.venv" ]]; then
    python3 -m venv "$project_root/.venv"
fi
"$project_root/.venv/bin/python" -m pip install --upgrade pip
"$project_root/.venv/bin/python" -m pip install -r "$project_root/python/requirements.txt"

make -C "$project_root" all test

printf '\nSetup complete. Start the leakage demo with:\n'
printf '  %s/scripts/start_demo.sh leakage\n' "$project_root"
