#!/usr/bin/env python3
"""
Convert TFF YAML configuration to JSON format for RP2040
"""

import yaml
import json
import sys
import os

def convert_tff_yaml_to_json(yaml_file, json_file):
    """Convert TFF YAML configuration to JSON format"""

    # Read the YAML file
    with open(yaml_file, 'r') as f:
        yaml_data = yaml.safe_load(f)

    # Convert to simplified JSON structure for RP2040
    json_data = {
        "settings": {
            "overlap_threshold_ms": 100
        },
        "mappings": []
    }

    # Process combos
    if 'combos' in yaml_data:
        for combo in yaml_data['combos']:
            if 'keys' in combo and 'outKeys' in combo:
                # Convert keys to array if it's a string
                keys = combo['keys']
                if isinstance(keys, str):
                    keys = keys.split()

                # Convert outKeys to array if it's a string
                out_keys = combo['outKeys']
                if isinstance(out_keys, str):
                    out_keys = [out_keys]

                json_data['mappings'].append({
                    "keys": keys,
                    "outKeys": out_keys
                })

    # Write JSON file
    with open(json_file, 'w') as f:
        json.dump(json_data, f, indent=2)

    print(f"Converted {yaml_file} to {json_file}")
    return True

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 yaml_to_json_converter.py <input.yaml> <output.json>")
        sys.exit(1)

    yaml_file = sys.argv[1]
    json_file = sys.argv[2]

    if not os.path.exists(yaml_file):
        print(f"Error: {yaml_file} not found")
        sys.exit(1)

    try:
        convert_tff_yaml_to_json(yaml_file, json_file)
    except Exception as e:
        print(f"Error converting file: {e}")
        sys.exit(1)