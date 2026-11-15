# Build PAC-MAN Championship Edition 2

The supported development targets are Linux x86-64 and Android ARM64. The tested
game is CE2 CUSA04924 v1.00, included in the CUSA06906 collection. Other collection
games are not selected by this launcher.

## Clone

```bash
git clone https://github.com/CrownParkComputing/pacman-ce2-recomp.git
cd pacman-ce2-recomp
```

Renderer dependencies under `gpu/third_party/` are vendored, including their
licences. No Git submodule setup is needed. Game files, prepared images, drivers,
Android signing keys, saves and build outputs stay outside Git.

## Linux prerequisites

Use a Linux x86-64 host with Clang supporting C++23, CMake 3.24+, Ninja, Git,
Python 3.10+, pkg-config, make, curl, xz and nasm. The renderer needs Vulkan
headers including Vulkan-Hpp, Vulkan loader, glslang, SDL3, fmt, Boost, robin-map,
xxHash, Zydis, zlib, X11 and Vulkan Memory Allocator headers.

On Ubuntu 24.04 the same installer used by CI installs system packages and builds
the missing SDL3/Vulkan headers. It uses sudo and installs into `/usr/local`:

```bash
bash .github/scripts/ci-deps.sh
export CC=clang-18 CXX=clang++-18
```

On another distribution install the equivalent development packages first.
A Vulkan-capable GPU/driver and desktop session are needed to play; source
compilation and most runtime checks work without a game or display.

## Build and check Linux

```bash
bash build.sh
```

Outputs are `out/pacman-ce2` and the inherited `out/tempest` runtime name. The
`out/tempest-setup` helper supplies the extractor's native XTS operation. The
build links minimal FFmpeg 9.0.2, unpacking the inherited Linux archive when its
options match, or downloading hash-checked source otherwise. Set
`TEMPEST_FFMPEG_PREBUILT=0` to build FFmpeg from source.

```bash
bash build.sh --test
(cd tests && python3 -m unittest discover -p 'test_*.py')
```

Tests requiring a missing game, graphics device or optional dependency may skip.
Focused CE2/timing checks can also be run after configuring the build:

```bash
cmake --build out/gpu --target ce2-profile-test ce2-profile-fex-test gpu-tick-cache-test render-loads-test wait-diagnostics-test
out/gpu/ce2-profile-test
out/gpu/ce2-profile-fex-test
out/gpu/gpu-tick-cache-test
out/gpu/render-loads-test
out/gpu/wait-diagnostics-test
```

## Supply your game

Use your own decrypted dump. Keep the whole collection under `game/`, with CE2
at `game/01PacManCE2`, and the shared `sce_module` directory available inside the
selected game. A supported local package can be extracted with:

```bash
python3 scripts/pkg_extract.py /path/to/your/CUSA06906.pkg --out game --xts-tool out/tempest-setup
```

The collection stores modules and trophy packages outside each game directory.
If the selected game lacks them, link the shared directories once:

```bash
ln -s ../sce_module game/01PacManCE2/sce_module
ln -s ../../sce_sys/trophy game/01PacManCE2/sce_sys/trophy
```

Only create those links if the corresponding path is absent. Keep any directory
already supplied by your dump. The selected directory must include:

```text
eboot.bin
sce_module/libc.prx
sce_module/libSceFios2.prx
sce_sys/param.sfo
Media/Modules/mono-ps4.sprx
Media/Modules/MonoAssembliesPS4.prx
Media/...
```

The tested CE2 executable SHA-256 is:
`909a1a46b20a1288966e7d036ce4e237698d2d71607fd53581e99bdf88732058`.
Preparation accepts this build directly. `--any-build` permits another executable
but does not make its addresses or game-specific patches compatible.

## Prepare and play

```bash
python3 run.py prepare --game game/01PacManCE2
./play-ce2.sh
```

`play-ce2.sh` selects the verified Linux 1080p profile and prepares the image when
needed. For another dump location or a bounded check:

```bash
./play-ce2.sh --game /path/to/01PacManCE2 --timeout 60
```

Prepared data is under `out/game/`; the original game files are not modified.
Saves are under `user/savedata/1/CUSA04924/AutoSave/`. Caches default to
`~/.cache/pacman-ce2/` and `out/shaders/`. Use `--user /path/to/profile` for an
isolated test profile. Linux and Android currently use separate saves.

The shared runtime retains `TEMPEST_*` environment names. Do not change the
Linux default to the inherited Neo/4K profile: its alias layout is not the
verified CE2 path.

## Android

Follow [android/README.md](../android/README.md) for dependency preparation,
ARM64/FEX compilation, Turnip packaging, signing, installation and device checks.
No path to another port's workspace is required.

## Automation and release scope

The GitHub workflow builds Linux and runs C/C++ and Python checks on pushes to
`main`, pull requests and manual dispatch. It contains no game files and does not
publish a binary release automatically. Android game staging and hardware tests
require local inputs and a connected device.

The inherited Tempest GUI setup, RecompLauncher metadata and Linux release
packager are not CE2 distribution instructions. Use this source-tree launcher
and the Android packaging script. Historical port notes and metadata are under
`docs/legacy/`; see [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for source
origins and dependency versions.
