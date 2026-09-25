#!/usr/bin/env python3
"""
Integration test for TFF2 Git pre-commit hook and install script.
Runs isolated tests in temporary git repositories.
"""
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

def run_cmd(cmd, cwd, check=True):
    res = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if check and res.returncode != 0:
        raise RuntimeError(f"Command failed ({res.returncode}): {' '.join(cmd)}\nStdout: {res.stdout}\nStderr: {res.stderr}")
    return res

def test_pre_commit_hook():
    repo_root = Path(__file__).resolve().parent.parent

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_repo = Path(tmpdir)

        # 1. Initialize git repo
        run_cmd(["git", "init"], cwd=tmp_repo)
        run_cmd(["git", "config", "user.name", "Test User"], cwd=tmp_repo)
        run_cmd(["git", "config", "user.email", "test@example.com"], cwd=tmp_repo)

        # 2. Copy .githooks, scripts, and schema
        shutil.copytree(repo_root / ".githooks", tmp_repo / ".githooks")
        shutil.copytree(repo_root / "scripts", tmp_repo / "scripts")
        shutil.copytree(repo_root / "schema", tmp_repo / "schema")
        (tmp_repo / ".clang-format").write_text((repo_root / ".clang-format").read_text())

        # 3. Test install_hooks.sh
        install_res = run_cmd(["bash", "scripts/install_hooks.sh"], cwd=tmp_repo)
        assert "Git hooks successfully configured" in install_res.stdout

        config_res = run_cmd(["git", "config", "core.hooksPath"], cwd=tmp_repo)
        assert config_res.stdout.strip() == ".githooks"

        # 4. Test clean staging with valid C++ file
        valid_cpp = tmp_repo / "test.cpp"
        valid_cpp.write_text("int main() {\n    return 0;\n}\n")
        # Format it cleanly
        subprocess.run(["clang-format", "-i", str(valid_cpp)], check=False)
        run_cmd(["git", "add", "test.cpp"], cwd=tmp_repo)

        hook_res = run_cmd(["bash", ".githooks/pre-commit"], cwd=tmp_repo, check=False)
        assert hook_res.returncode == 0, f"Expected 0, got {hook_res.returncode}: {hook_res.stderr}"
        assert "Pre-commit checks passed" in hook_res.stdout

        # 5. Test unformatted C++ file rejection
        unformatted_cpp = tmp_repo / "bad_format.cpp"
        unformatted_cpp.write_text("int    badFormat(   int  x){return   x+1;}\n")
        run_cmd(["git", "add", "bad_format.cpp"], cwd=tmp_repo)

        hook_bad_cpp = run_cmd(["bash", ".githooks/pre-commit"], cwd=tmp_repo, check=False)
        assert hook_bad_cpp.returncode != 0, "Hook should have failed on unformatted C++"
        assert "Format check failed" in hook_bad_cpp.stdout or "Format check failed" in hook_bad_cpp.stderr

        # Unstage bad file
        run_cmd(["git", "rm", "-f", "bad_format.cpp"], cwd=tmp_repo)

        # 6. Test valid YAML file
        valid_yaml = tmp_repo / "config.yaml"
        valid_yaml.write_text("combos:\n  - keys: a + s\n    outKeys: esc\n    timeout_ms: 50\n")
        run_cmd(["git", "add", "config.yaml"], cwd=tmp_repo)

        hook_valid_yaml = run_cmd(["bash", ".githooks/pre-commit"], cwd=tmp_repo, check=False)
        assert hook_valid_yaml.returncode == 0, f"Expected 0, got {hook_valid_yaml.returncode}: {hook_valid_yaml.stderr}"

        # 7. Test invalid YAML schema rejection (e.g. timeout_ms: -5)
        bad_yaml = tmp_repo / "bad_config.yaml"
        bad_yaml.write_text("combos:\n  - keys: a + s\n    outKeys: esc\n    timeout_ms: -5\n")
        run_cmd(["git", "add", "bad_config.yaml"], cwd=tmp_repo)

        hook_bad_yaml = run_cmd(["bash", ".githooks/pre-commit"], cwd=tmp_repo, check=False)
        assert hook_bad_yaml.returncode != 0, "Hook should have failed on invalid YAML schema"

        print("✓ All Git pre-commit hook integration tests passed!")

if __name__ == "__main__":
    test_pre_commit_hook()
