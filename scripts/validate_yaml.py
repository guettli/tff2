#!/usr/bin/env python3
"""
Validates YAML files for syntax and against schema/tff-schema.json.
Used by Git pre-commit hook and CI checks. Supports both file paths and stdin.
"""
import sys
import json
from pathlib import Path

def is_tff_config(filepath: str, data, raw_text: str = "") -> bool:
    if not isinstance(data, dict):
        return False
    tff_keys = {"combos", "tap_hold", "layers", "settings", "tap_dance", "leader", "one_shot", "auto_shift", "mouse"}
    if any(k in tff_keys for k in data.keys()):
        return True
    if "tff-schema.json" in raw_text[:500]:
        return True
    if filepath.startswith("config/") or "/config/" in filepath:
        return True
    return False

def print_help():
    print("Usage: validate_yaml.py [--filename <name> -] [<file1.yaml> ...]")
    print("Validates YAML files for syntax and against schema/tff-schema.json.")
    print("Options:")
    print("  --filename <name> -   Read YAML content from standard input")
    print("  -h, --help            Show this help message")

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print_help()
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
    args = sys.argv[1:]

    # Check for stdin mode: --filename <name> -
    if len(args) == 3 and args[0] == "--filename" and args[2] == "-":
        filename = args[1]
        try:
            raw_text = sys.stdin.read()
        except Exception as e:
            print(f"Error reading from stdin: {e}", file=sys.stderr)
            sys.exit(1)
        
        try:
            docs = list(yaml.safe_load_all(raw_text))
        except Exception as e:
            print(f"YAML Syntax Error in {filename}: {e}", file=sys.stderr)
            sys.exit(1)

        for doc in docs:
            if doc is None:
                continue
            if is_tff_config(filename, doc, raw_text):
                if schema and has_jsonschema:
                    try:
                        jsonschema.validate(instance=doc, schema=schema)
                    except jsonschema.ValidationError as e:
                        print(f"Schema Validation Error in {filename}: {e.message}", file=sys.stderr)
                        if e.path:
                            print(f"  at path: {' -> '.join(str(p) for p in e.path)}", file=sys.stderr)
                        failed = True
                    except Exception as e:
                        print(f"Validation Error in {filename}: {e}", file=sys.stderr)
                        failed = True
                elif not has_jsonschema:
                    print(f"! Warning: jsonschema not installed. Skipping schema validation for {filename}.", file=sys.stderr)
        if failed:
            sys.exit(1)
        sys.exit(0)

    for arg in args:
        if arg in ("-h", "--help"):
            continue
        p = Path(arg)
        if not p.exists():
            print(f"Error: File not found: {arg}", file=sys.stderr)
            failed = True
            continue
        if p.is_dir():
            continue

        try:
            with open(p, "r", encoding="utf-8") as f:
                raw_text = f.read()
        except Exception as e:
            print(f"Error reading {arg}: {e}", file=sys.stderr)
            failed = True
            continue

        try:
            docs = list(yaml.safe_load_all(raw_text))
        except Exception as e:
            print(f"YAML Syntax Error in {arg}: {e}", file=sys.stderr)
            failed = True
            continue

        for doc in docs:
            if doc is None:
                continue
            if is_tff_config(str(p), doc, raw_text):
                if schema and has_jsonschema:
                    try:
                        jsonschema.validate(instance=doc, schema=schema)
                    except jsonschema.ValidationError as e:
                        print(f"Schema Validation Error in {arg}: {e.message}", file=sys.stderr)
                        if e.path:
                            print(f"  at path: {' -> '.join(str(p) for p in e.path)}", file=sys.stderr)
                        failed = True
                    except Exception as e:
                        print(f"Validation Error in {arg}: {e}", file=sys.stderr)
                        failed = True
                elif not has_jsonschema:
                    print(f"! Warning: jsonschema not installed. Skipping schema validation for {arg}.", file=sys.stderr)

    if failed:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
