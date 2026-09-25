#!/usr/bin/env bash
# ==============================================================================
# scripts/install_hooks.sh - Install Git hooks for TFF2
# Configures core.hooksPath to point to .githooks in the local repository.
# ==============================================================================
set -euo pipefail

if ! git rev-parse --is-inside-work-tree &>/dev/null; then
    echo "Error: Not inside a Git repository. Cannot install Git hooks." >&2
    exit 1
fi

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "${REPO_ROOT}"

if [[ ! -d ".githooks" ]]; then
    echo "Error: .githooks directory not found at ${REPO_ROOT}/.githooks" >&2
    exit 1
fi

chmod +x .githooks/* 2>/dev/null || true
git config core.hooksPath .githooks

echo -e "\033[1;32m✓ Git hooks successfully configured (.githooks)\033[0m"
echo "  Pre-commit checks will now run automatically on staged C/C++ and YAML files."
