#!/usr/bin/env bash
# This script symbolically links this project's root directory into the nexuslua plugin folder,
# allowing it to be recognized as an installed plugin for development purposes.

# Determine operating system and set plugin directory accordingly
case "$(uname -s)" in
  Linux*)     plugins_dir=~/.local/share/nexuslua/plugins;;
  Darwin*)    plugins_dir=~/Library/Application\ Support/nexuslua/plugins;;
  MINGW*)     plugins_dir="$APPDATA/nexuslua/plugins";;
  *)          echo "Unknown operating system"; exit 1;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$plugins_dir"

# Read plugin name from specification file
plugin_spec_file="$SCRIPT_DIR/nexuslua_plugin.toml"
if [ -f "$plugin_spec_file" ]; then
  # Parse the displayName from the TOML file.
  plugin_name=$(grep '^displayName =' "$plugin_spec_file" | sed 's/displayName = "\(.*\)"/\1/')
  echo "Plugin name: '$plugin_name'"
else
  echo "Error: Missing $plugin_spec_file."
  exit 1
fi

# Create the symbolic link
if [[ "$(uname -s)" == MINGW* ]]; then
  # On Windows, this script works ONLY in "Git Bash" (MSYSTEM=MINGW64).
  # It fails in MSYS2 shells (like UCRT64, MSYS) due to how they
  # handle PowerShell elevation and path conversion.
  if [[ "$MSYSTEM" != "MINGW64" ]]; then
    echo "Error: Unsupported Windows Shell Environment Detected."
    echo "Please run the script again from a 'Git Bash' prompt, which is installed with Git for Windows."
    exit 1
  fi

  powershell -Command "Start-Process cmd -ArgumentList '/c mklink /D \"$(cygpath -w "$plugins_dir\\$plugin_name")\" \"$(cygpath -w "$SCRIPT_DIR")\"' -Verb RunAs"
else
  ln -sfn "$SCRIPT_DIR" "$plugins_dir/$plugin_name"
fi

echo "Successfully created symbolic link: $plugins_dir/$plugin_name"
