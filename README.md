### What is in this repository?
-------------

This repository contains tooling designed to repackage Celeste's game textures in ASTC 4x4 LDR format, this is meant to
be used in conjunction with a [game patch](https://github.com/JohnnyonFlame/FNAPatches) that allows the game to actually make use of this file format, providing
improved rendering performance and lower VRAM pressure.

### Usage:
-------------

By default, the application will save your textures into `out/` on your working folder, if you want to install in-place, use the `--install` option.

- Example:
```bash
./celeste-repacker /path/to/celeste/Content/Graphics --install
```

### Compiling (for ARM deployment):
-------------

```bash
git submodule update --init --recursive
mkdir build
cmake -B build/ -DISA_NEON=ON -DCMAKE_BUILD_TYPE=Release
make -C build $($(nproc)+1)
```

You can also use the [CMAKE_TOOLCHAIN_FILE variable](https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html) to enable cross-compilation.

### LICENSE:
-------------

The files in this repository are free software licensed under Apache 2.0, read the [License File](LICENSE.txt) for reference.
