#!/usr/bin/env python3
"""
Integration test for TFF2 Git pre-commit hook and install script.
Runs isolated tests in temporary git repositories, executing real git commits.
"""
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

def find_clang_format():
    candidates = ["clang-format", "clang-format-18", "clang-format-17", "clang-format-16",
                  "clang-format-15", "clang-format-14", str(Path.home() / ".local" / "bin" / "clang-format")]
    for c in candidates:
        if shutil.which(c):
            return c
    return None

def has_jsonschema():
    try:
        import jsonschema
        return True
    except ImportError:
        return False

def run_cmd(cmd, cwd, check=True):
    res = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if check and res.returncode != 0:
        raise RuntimeError(f"Command failed ({res.returncode}): {' '.join(cmd)}\nStdout: {res.stdout}\nStderr: {res.stderr}")
    return res

def test_pre_commit_hook():
    repo_root = Path(__file__).resolve().parent.parent
    clang_fmt = find_clang_format()
    have_schema = has_jsonschema()

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

        # 4. Test real git commit with valid C++ file
        valid_cpp = tmp_repo / "test.cpp"
        valid_cpp.write_text("int main() {\n    return 0;\n}\n")
        if clang_fmt:
            subprocess.run([clang_fmt, "-i", str(valid_cpp)], check=False)
        run_cmd(["git", "add", "test.cpp"], cwd=tmp_repo)

        commit_res = run_cmd(["git", "commit", "-m", "Commit valid C++"], cwd=tmp_repo, check=False)
        assert commit_res.returncode == 0, f"Expected clean commit, got: {commit_res.stderr}\n{commit_res.stdout}"

        # 5. Test partial staging: staged content is clean, but working tree has unformatted edits
        # Working tree modification must NOT break commit when staged content is clean
        valid_cpp.write_text("int    unformatted_in_working_tree(  int x ){return  x;}\n")
        # Notice we do NOT run git add
        empty_commit = run_cmd(["git", "commit", "--allow-empty", "-m", "Commit with dirty working tree"], cwd=tmp_repo, check=False)
        assert empty_commit.returncode == 0, "Partial staging check failed: working tree edits interfered with staged commit"
        # Reset working tree file
        run_cmd(["git", "checkout", "test.cpp"], cwd=tmp_repo)

        # 6. Test unformatted C++ staged file rejection
        if clang_fmt:
            bad_cpp = tmp_repo / "bad_format.cpp"
            bad_cpp.write_text("int    badFormat(   int  x){return   x+1;}\n")
            run_cmd(["git", "add", "bad_format.cpp"], cwd=tmp_repo)

            commit_bad_cpp = run_cmd(["git", "commit", "-m", "Bad C++"], cwd=tmp_repo, check=False)
            assert commit_bad_cpp.returncode != 0, "git commit should have failed on unformatted C++"
            assert "Format check failed" in commit_bad_cpp.stdout or "Format check failed" in commit_bad_cpp.stderr
            # Unstage bad file
            run_cmd(["git", "rm", "-f", "bad_format.cpp"], cwd=tmp_repo)
        else:
            print("Note: clang-format not available, skipped bad C++ rejection test")

        # 7. Test non-combo YAML file with 'tff' in name (e.g. CI workflow) is NOT rejected
        # This tests against false positives reported in review
        workflow_dir = tmp_repo / ".github" / "workflows"
        workflow_dir.mkdir(parents=True)
        ci_yaml = workflow_dir / "tff-ci.yml"
        ci_yaml.write_text("name: TFF CI\non: [push]\njobs:\n  test:\n    runs-on: ubuntu-latest\n    steps:\n      - run: echo hello\n")
        run_cmd(["git", "add", ".github/workflows/tff-ci.yml"], cwd=tmp_repo)

        commit_ci = run_cmd(["git", "commit", "-m", "Add CI workflow"], cwd=tmp_repo, check=False)
        assert commit_ci.returncode == 0, f"False positive on CI yaml: {commit_ci.stderr}\n{commit_ci.stdout}"

        # 8. Test valid TFF configuration YAML file
        valid_yaml = tmp_repo / "config.yaml"
        valid_yaml.write_text("combos:\n  - keys: a + s\n    outKeys: esc\n    timeout_ms: 50\n")
        run_cmd(["git", "add", "config.yaml"], cwd=tmp_repo)

        commit_yaml = run_cmd(["git", "commit", "-m", "Valid config"], cwd=tmp_repo, check=False)
        assert commit_yaml.returncode == 0, f"Expected clean commit for YAML: {commit_yaml.stderr}"

        # 9. Test invalid YAML schema rejection (e.g. timeout_ms: -5)
        if have_schema:
            bad_yaml = tmp_repo / "bad_config.yaml"
            bad_yaml.write_text("combos:\n  - keys: a + s\n    outKeys: esc\n    timeout_ms: -5\n")
            run_cmd(["git", "add", "bad_config.yaml"], cwd=tmp_repo)

            commit_bad_yaml = run_cmd(["git", "commit", "-m", "Bad YAML"], cwd=tmp_repo, check=False)
            assert commit_bad_yaml.returncode != 0, "git commit should have failed on invalid YAML schema"
            run_cmd(["git", "rm", "-f", "bad_config.yaml"], cwd=tmp_repo)
        else:
            print("Note: jsonschema not available, skipped schema failure test")

        print("✓ All Git pre-commit hook integration tests passed!")

if __name__ == "__main__":
    test_pre_commit_hook()
