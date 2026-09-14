#!/usr/bin/env python3
"""
Convert TFF YAML configuration to JSON format for RP2040
"""

import json
import sys
import os

def simple_yaml_to_json(yaml_content):
    """Simple parser for TFF YAML format"""

    # Simple state tracking
    lines = yaml_content.strip().split('\n')
    mappings = []

    in_combos = False
    current_combo = None

    for line in lines:
        line = line.strip()

        # Skip empty lines and comments
        if not line or line.startswith('#'):
            continue

        # Check for combos section
        if line.startswith('combos:'):
            in_combos = True
            continue

        # Process combo entries
        if in_combos:
            if line.startswith('- keys:'):
                # Start new combo
                keys_part = line[8:].strip()  # Remove "- keys: "
                current_combo = {'keys': keys_part}
            elif line.startswith('outKeys:') and current_combo:
                # Add outKeys to current combo
                outkeys_part = line[9:].strip()  # Remove "outKeys: "
                current_combo['outKeys'] = outkeys_part
                mappings.append(current_combo)
                current_combo = None

    return {
        'settings': {
            'overlap_threshold_ms': 100
        },
        'mappings': mappings
    }

def convert_file(input_file, output_file):
    """Convert YAML file to JSON"""

    with open(input_file, 'r') as f:
        yaml_content = f.read()

    json_data = simple_yaml_to_json(yaml_content)

    with open(output_file, 'w') as f:
        json.dump(json_data, f, indent=2)

    print(f"Converted {input_file} to {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 convert_tff_config.py <input.yaml> <output.json>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found")
        sys.exit(1)

    try:
        convert_file(input_file, output_file)
    except Exception as e:
        print(f"Error converting file: {e}")
        sys.exit(1)