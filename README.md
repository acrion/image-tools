This project provides a robust C++ toolkit for image file I/O, designed to integrate seamlessly with the [acrion/image](https://github.com/acrion/image) container library. It bundles the powerful [ImageMagick](https://imagemagick.org) library for handling a wide range of common image formats and NASA's [cfitsio](https://heasarc.gsfc.nasa.gov/fitsio) library for native support of the FITS astronomical data format.

## Highlights

*   **Modern C++ library** for reading/writing images.
*   **ImageMagick backend** for common formats (JPEG, PNG, TIFF, BMP, TGA, etc.).
*   **FITS via cfitsio**: uses the reference implementation, which is vendored (included directly) in this repository.
*   **Container-first design**: interoperates with the separate [acrion/image](https://github.com/acrion/image) container library.
*   **Plugin mode**: can be loaded by [nexuslua](https://github.com/acrion/nexuslua) to process images asynchronously via message passing.
*   **Zero-drama Windows runtime**: at build time, the official ImageMagick build recipe (from the MSYS2 MINGW packages) is fetched, modified to support 32-bit depth, built, and then copied next to your binaries, including all modules and `.la` files; no global installation is required.

> **Used by acrionphoto:** This plugin provides the default image I/O for `acrionphoto`.  
> `acrionphoto` will run without it, but **cannot open or save images** until an I/O plugin is installed.

The I/O role is **replaceable**: any plugin can provide image I/O as long as its `main.lua` declares the expected messages and parameters (see `loadpath`/`savepath` conventions below). This keeps I/O modular and allows alternative or additional providers.

## Supported formats & notes

*   **Read**: TIFF (`.tif/.tiff`), FITS (`.fit/.fits`), PNG (`.png`), JPEG (`.jpg/.jpeg`), BMP (`.bmp`), TARGA (`.tga`), DICOM (`.dcm`) — subject to ImageMagick delegates present at runtime.
*   **Write**: TIFF, FITS, PNG, JPEG, BMP, TARGA.
*   **Bit depths** (read/write via ImageMagick): 8/16/32/64-bit unsigned per channel.
*   **Floating-point** images are supported via FITS (read/write); colored FITS output is currently not supported.
*   Format support ultimately depends on which ImageMagick coders are available in your build (this repo takes care of shipping them on Windows).

## How ImageMagick is integrated

*   The project **builds or stages** the official ImageMagick packages per platform and exposes them via **IMPORTED** CMake targets.
*   On **Windows (MSYS2 / UCRT64)**:

    *   We build from `mingw-w64-imagemagick` (with a modified configuration, primarily changing the quantum depth to 32-bit) and copy the runtime **DLLs**, **modules**, and `.la` files into your build and plugin directories.
    *   Modules are flattened to:

        ```
        <your-binary-dir>/
          modules-Q32/{coders,filters}/...
          config-Q32/...
        ```

        (No versioned `ImageMagick-7.x.y` parent directory.)
    *   At runtime, the library sets:

        *   `MAGICK_CODER_MODULE_PATH` → `modules-Q32/coders`
        *   `MAGICK_FILTER_MODULE_PATH` → `modules-Q32/filters`
            You don’t have to set these yourself.
*   On **Linux**:

    *   The necessary ImageMagick shared libraries are copied directly into the build's output directory. The executable's `RPATH` is then patched to ensure it loads these local libraries instead of any system-wide installations. All required ImageMagick modules are linked directly into the main shared library, which simplifies runtime configuration by eliminating the need to set module search paths.

*   On **macOS**

    *   macOS support is currently under development. The planned approach is to adapt the official Homebrew formula for ImageMagick to meet this project's specific requirements (e.g., Q32 quantum depth). This is achieved by creating a custom local formula file and instructing Homebrew to build ImageMagick from source with the necessary modifications, analogous to the process on other platforms.

On all platforms, the library programmatically sets `MAGICK_HOME` and other necessary environment variables at runtime. This configuration is handled internally and is confined to the process using the library, ensuring that it does not interfere with the global user environment or other applications.

## Building

### Prerequisites

*   **CMake ≥ 3.25**
*   A modern C++ compiler
*   **Ninja** (recommended)
*   Platform tooling:

    *   **Windows**: MSYS2 UCRT64 shell (`ucrt64.exe`)
    *   **Linux (Arch-based)**: `pacman` (for the ImageMagick build path used here)
    *   **macOS**: Homebrew (for toolchain/Qt if needed)

> This library is also built automatically by the umbrella repo [nexuslua-build](https://github.com/acrion/nexuslua-build). If you plan to build the full stack (nexuslua, plugins, optional GUI), start there.

### Configure & build (standalone)

```bash
# From repo root
cmake -B build -D CMAKE_BUILD_TYPE=Release acrion_image_tools/
cmake --build build --parallel
```

Artifacts are written to `cmake-build-*/bin` and the project root (for the plugin build results). On Windows, you’ll see `libMagick*` DLLs and the `modules-Q32`/`config-Q32` folders placed automatically next to the binaries.

### Tests

A minimal GoogleTest executable (`acrion_image_tools_test`) is produced. You can run it directly from `cmake-build-*/bin`.

## Using the C++ API

```cpp
#include "acrion_image_tools/io.hpp"
#include "acrion/image/bitmap.hpp"

using acrion::imagetools::io;

int main() {
  std::string warning;
  auto bmp = io::Read(L"input.tif", warning);   // Returns shared_ptr<acrion::image::Bitmap>
  if (!warning.empty()) {
    // handle non-fatal warnings from ImageMagick
  }

  // ... process bmp ...

  io::Write(*bmp, L"output.png", warning);
  return 0;
}
```

### Version helpers

```cpp
#include "acrion_image_tools/version_acrion_image_tools.hpp"

std::string lib_version  = acrion::imagetools::GetVersion();
std::string im_version   = acrion::imagetools::GetImageMagickVersion();
std::string fits_version = acrion::imagetools::GetCfitsioVersion();
```

## Using as a nexuslua plugin

The built repository can be installed or directly symlinked as a [nexuslua](https://github.com/acrion/nexuslua) plugin. It exposes high-performance C++ functions for image processing, which are defined and made available as a nexuslua agent, communicating via concurrent messages.

### Available Operations

The plugin provides a range of operations accessible through `nexuslua` messages:

*   **File I/O**: `CallOpenImageFile`, `CallSaveImageFile`
*   **Image Manipulation**: `CallSwap`, `CallCopyLeftToRight`, `CallCopyRightToLeft`, `CallInvertImage`
*   **Arithmetic**: `CallSubtract*` (no wrap, wrap, absolute difference)
*   **Pixel Operations**: `GetPixelValueOfChannel`, `DrawWhitePixel`, etc.

### Scripting in nexuslua

In addition to the compiled C++ functions, the plugin architecture allows for direct pixel manipulation in nexuslua via memory access functions (`peek`, `poke`), enabling rapid prototyping of custom algorithms. For more details on the `nexuslua` scripting API, please refer to the [official nexuslua documentation](https://nexuslua.org).

### Dev install into nexuslua

```bash
./install-development-plugin.sh
```

This script creates a symbolic link from the project directory to your local `nexuslua` plugin folder. This is a convenient way to test changes live without needing to reinstall the plugin after every build.

## Plugin metadata (`nexuslua_plugin.toml`)

During build, a `nexuslua_plugin.toml` is written into the plugin root with metadata consumed by the `acrionphoto` Plugin Manager and by `nexuslua` tooling. Example:

```toml
displayName = "acrion image tools"
version = "1.0.246"
isFreeware = true
description = "A set of essential tools for basic image manipulation."
urlHelp = "https://github.com/acrion/image-tools"
urlLicense = "https://github.com/acrion/image-tools/blob/main/LICENSE"
urlDownloadLinux = "https://github.com/acrion/image-tools/releases/download/1.0.246/image-tools-Linux.zip"
urlDownloadWindows = "https://github.com/acrion/image-tools/releases/download/1.0.246/image-tools-Windows.zip"
#urlDownloadDarwin = "https://github.com/acrion/image-tools/releases/download/1.0.246/image-tools-Darwin.zip"
```

* The macOS download URL is commented while the macOS build is under refactoring.
* If `isFreeware = false` and `urlPurchase` is provided, `acrionphoto` shows **“Get License Key…”**; it also exposes **“Install key or other files…”** for copying data into the plugin’s `persistent/` folder.
* The planned `acrion/nexuslua-plugins` repository will hold **only** the URLs of such TOML files; the files themselves live with each plugin.

## Project structure (selected)

```
main.lua                      # nexuslua message definitions and C++/Lua bridge
acrion_image_tools/
  nexuslua_plugin.toml.template # Template for nexuslua plugin metadata file
  CMakeLists.txt                # Main build logic, including ExternalProject for ImageMagick
  io.hpp|cpp                    # Public API for image I/O via ImageMagick + FITS
  fits.hpp|cpp                  # FITS reading/writing using the vendored cfitsio library
  imagemagick.hpp               # ImageMagick headers/config (Q32 depth, HDRI toggle)
  main.cpp                      # C++ entry points for functions exposed to nexuslua
  im/                           # ImageMagick build glue (PKGBUILDs, patches, scripts)
  cfitsio/                      # vendored cfitsio reference implementation
  version_*                     # Version query utilities (library, IM, cfitsio)
```

## Troubleshooting

*   **“NoDecodeDelegateForThisImageFormat 'JPEG'”**
    On Windows, ensure the runtime directory contains `modules-Q32/coders/jpeg.dll` **and** `jpeg.la` next to your application binary. Set the environment variable `MAGICK_DEBUG=Module,Coder` to see verbose module lookup paths.
*   **Running from a different working directory**
    The library derives necessary paths from the **current binary's location**, not the current working directory (CWD). As long as the dependent libraries and modules are placed next to your executable, they will be found.


---

## Community

Have questions about the library, want to share what you've built, or discuss image processing techniques? Join our community on Discord!

💬 **[Join the acrion image Discord Server](https://discord.gg/FeWM5s5jKm)**

---

## Licensing

acrion image-tools is dual-licensed to support both open-source development and commercial use.

1.  **AGPL v3 or later**: For use in open-source projects that are compatible with the AGPL.
2.  **Commercial License**: For integration into proprietary applications or for cases where AGPLv3 terms cannot be met.

I would like to emphasize that offering a dual license does not restrict users of the normal open-source license (including commercial users).
The dual licensing model is designed to support both open-source collaboration and commercial integration needs.
For commercial licensing inquiries, please contact us at [https://acrion.ch/sales](https://acrion.ch/sales).

### Third-Party Libraries

`acrion image-tools` incorporates or links against several third-party libraries. Their licenses apply to their respective components and must be respected.

*   **cfitsio**: The source code is included directly in this repository. It is released under a permissive, public-domain-like license granted by NASA. The full license text is available in `acrion_image_tools/cfitsio/License.txt`.
*   **ImageMagick**: This library is downloaded and built at compile time. It is distributed under the ImageMagick License, which is a permissive, Apache 2.0-style license. You can find more details on the [official ImageMagick website](https://imagemagick.org/script/license.php).
