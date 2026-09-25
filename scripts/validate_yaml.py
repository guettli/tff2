#!/usr/bin/env python3
"""
Validates YAML files for syntax and against schema/tff-schema.json.
Used by Git pre-commit hook and CI checks.
"""
import sys
import json
from pathlib import Path

def is_tff_config(filepath: str, data) -> bool:
    if not isinstance(data, dict):
        return False
    tff_keys = {"combos", "tap_hold", "layers", "settings", "tap_dance", "leader", "one_shot", "auto_shift", "mouse"}
    if any(k in tff_keys for k in data.keys()):
        return True
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            head = f.read(500)
            if "tff-schema.json" in head:
                return True
    except Exception:
        pass
    if "config/" in filepath:
        return True
    return False

def main():
    if len(sys.argv) < 2:
        print("Usage: validate_yaml.py <file1.yaml> [file2.yaml ...]")
        sys.exit(0)

    repo_root = Path(__file__).resolve().parent.parent
    schema_path = repo_root / "schema" / "tff-schema.json"
    schema = None
    if schema_path.exists():
        try:
            with open(schema_path, "r", encoding="utf-8") as f:
                schema = json.load(f)
        except Exception as e:
            print(f"Error loading schema {schema_path}: {e}", file=sys.stderr)
            sys.exit(1)

    try:
        import yaml
    except ImportError:
        print("Warning: PyYAML not installed. Skipping YAML validation.", file=sys.stderr)
        sys.exit(0)

    has_jsonschema = False
    try:
        import jsonschema
        has_jsonschema = True
    except ImportError:
        pass

    failed = False
    for arg in sys.argv[1:]:
        p = Path(arg)
        if not p.exists() or p.is_dir():
            continue
        try:
            with open(p, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f)
        except Exception as e:
            print(f"YAML Syntax Error in {arg}: {e}", file=sys.stderr)
            failed = True
            continue

        if data is None:
            continue

        if is_tff_config(str(p), data):
            if schema and has_jsonschema:
                try:
                    jsonschema.validate(instance=data, schema=schema)
                except jsonschema.ValidationError as e:
                    print(f"Schema Validation Error in {arg}: {e.message}", file=sys.stderr)
                    if e.path:
                        print(f"  at path: {' -> '.join(str(p) for p in e.path)}", file=sys.stderr)
                    failed = True
                except Exception as e:
                    print(f"Validation Error in {arg}: {e}", file=sys.stderr)
                    failed = True

    if failed:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
