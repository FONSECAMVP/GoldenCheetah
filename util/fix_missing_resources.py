#!/usr/bin/env python3
"""
Script to find and create missing resource files for GoldenCheetah build
"""

import subprocess
import re
import os
from pathlib import Path

def create_placeholder_svg(filepath):
    """Create a simple placeholder SVG"""
    svg_content = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect width="24" height="24" fill="#cccccc"/>
  <text x="12" y="12" text-anchor="middle" dominant-baseline="middle" font-size="8" fill="#666">?</text>
</svg>'''
    with open(filepath, 'w') as f:
        f.write(svg_content)
    print(f"Created placeholder SVG: {filepath}")

def create_placeholder_png(filepath):
    """Copy an existing PNG as placeholder"""
    source = "src/Resources/images/home.png"
    if os.path.exists(source):
        subprocess.run(['cp', source, filepath], check=False)
        print(f"Created placeholder PNG: {filepath}")
    else:
        # Create empty file
        Path(filepath).touch()
        print(f"Created empty PNG: {filepath}")

def find_and_create_missing_files(max_iterations=50):
    """Find missing files and create placeholders"""
    base_dir = "/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah/"
    
    for iteration in range(max_iterations):
        # Run make and capture output
        result = subprocess.run(
            ['make', 'GoldenCheetah'],
            cwd=f'{base_dir}build',
            capture_output=True,
            text=True
        )
        
        # Look for "No rule to make target" errors
        pattern = r"No rule to make target '([^']+)'"
        matches = re.findall(pattern, result.stderr)
        
        if not matches:
            print(f"\nNo more missing files found after {iteration} iterations!")
            return True
        
        # Process first missing file
        missing_file = matches[0]
        print(f"\nIteration {iteration + 1}: Found missing file: {missing_file}")
        
        # Create directory if needed
        dir_path = os.path.dirname(missing_file)
        os.makedirs(dir_path, exist_ok=True)
        
        # Create placeholder based on file type
        if missing_file.endswith('.svg'):
            create_placeholder_svg(missing_file)
        elif missing_file.endswith('.png'):
            create_placeholder_png(missing_file)
        elif missing_file.endswith('.xml'):
            # Create minimal XML
            with open(missing_file, 'w') as f:
                f.write('<root></root>')
            print(f"Created placeholder XML: {missing_file}")
        else:
            # Create empty file
            Path(missing_file).touch()
            print(f"Created empty file: {missing_file}")
    
    print(f"\nReached maximum iterations ({max_iterations})")
    return False

if __name__ == "__main__":
    print("Finding and creating missing resource files...")
    success = find_and_create_missing_files()
    
    if success:
        print("\n✅ All missing files created successfully!")
        print("You can now run: cd build && make GoldenCheetah")
    else:
        print("\n⚠️  Some files may still be missing. Check build output.")
