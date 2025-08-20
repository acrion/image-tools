#!/bin/bash
# This is a static script to build ImageMagick on MSYS2.
set -e

CONFIG_FILE="$HOME/.makepkg_mingw.conf"

if [ -r "${CONFIG_FILE}" ]; then
  set -a
  source "${CONFIG_FILE}"
  set +a
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PATCH_FILE="${SCRIPT_DIR}/PKGBUILD-mingw.patch"

echo "--- Starting ImageMagick MSYS2 build ---"
echo "Build script location: ${SCRIPT_DIR}"
echo "Patch file location: ${PATCH_FILE}"

# The script is executed from the ExternalProject's source dir (imagemagick-build-msys2)
cd mingw-w64-imagemagick

echo "Applying patch to PKGBUILD..."
patch < "${PATCH_FILE}"

echo "Running makepkg-mingw..."
makepkg-mingw --nodeps --noconfirm --nocheck

echo "--- ImageMagick MSYS2 build finished successfully ---"
