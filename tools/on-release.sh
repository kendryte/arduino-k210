#!/usr/bin/env bash
# Compatibility entry point for the release workflow.
set -euo pipefail
export GH_TOKEN="${GH_TOKEN:-${GITHUB_TOKEN:-}}"
exec python3 "$(dirname "$0")/release.py" --publish "$@"
