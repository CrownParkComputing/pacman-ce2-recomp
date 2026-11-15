# PAC-MAN CE2 on Android

Tested on Retroid Pocket Flip2 (Snapdragon 865, Adreno 650, Android 13).
The application ID is `com.crownparkcomputing.pacmance2`, activity
`.PacmanActivity`, label `PAC-MAN CE2`. It installs separately from Tempest.
Game files stay outside the APK.

## Host prerequisites

Start with the Linux development dependencies in [docs/building.md](../docs/building.md).
Install a JDK 17 or newer, `zip`, Android SDK platform 33 or newer, SDK build-tools
and platform-tools, and an ARM64-capable NDK. The tested NDK is
`30.0.14904198`; the native build targets API 31. Point the variables at your SDK:

```bash
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
export ANDROID_NDK_HOME="$ANDROID_SDK_ROOT/ndk/30.0.14904198"
export PATH="$ANDROID_SDK_ROOT/platform-tools:$PATH"
```

## Prepare dependencies

```bash
bash tools/setup_android_deps.sh
source out/android-env.sh
```

This stages only Boost, robin-map, Vulkan/Vulkan-Hpp and VMA headers from the host,
builds hash-checked SDL3 3.4.16 for ARM64, checks out pinned libadrenotools and
builds minimal Android FFmpeg. Outputs remain under `out/`. For nonstandard host
header prefixes set `TEMPEST_ANDROID_HEADER_PREFIXES=/prefix/one:/prefix/two`.
It never copies the host's libc headers into the NDK include path.

## Build ARM64 runtime

```bash
bash tools/build_android_fex.sh --ndk
```

The script downloads FEX at `f2b679f6028ce1c38875233aecfcf5d3f8ebecec`,
applies `android/fexcore-only.patch`, fetches its selected submodules and builds
`out/android-fex-ndk/android-libs/libmain.so` plus the execution checks.
It refuses a checkout at another revision. `TEMPEST_BUILD_JOBS=4` reduces build
parallelism on smaller hosts.

An existing patched FEX checkout can be reused with `TEMPEST_FEX_SOURCE` and
`TEMPEST_FEX_SOURCE_READONLY=1`; this is optional. Source the generated environment
again in each new shell before rebuilding or packaging.

## Driver and signing

Use the Standard Adreno 6xx/7xx [Turnip build dated November 11, 2026](https://github.com/The412Banner/Banners-Turnip/releases/tag/v27.0.0-20261111-r2),
`Turnip-v27.0.0-20261111-r2.zip`. ZIP SHA-256:
`bcc118d8a8bb4206f2012c1941252f46eb92b3da5cc579af368ac56fb2769271`.
Its library SHA-256 is
`472c1d0b4b415229facc3015e73dd4f5c8298cdbc7207d5df8b60edfda64039e`.
Keep the ZIP under `out/` or outside the checkout.

This build contains Mesa's [KGSL zero-timeout fix](https://github.com/chaotic-cx/mesa-mirror/commit/e984ef29).
Older R5 builds can block on nonblocking completion checks and cause slow gameplay.
The driver ZIP and library are not checked into this repository;
`assets/driver-source.json` records exact APK provenance.

For local development create a debug signing key once, if you do not have one:

```bash
mkdir -p "$HOME/.android"
keytool -genkeypair -keystore "$HOME/.android/debug.keystore" -storepass android -keypass android -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=Android Debug'
```

Keep using the same key to update an existing installation without uninstalling.
Do not commit signing keys.

```bash
bash tools/package_android.sh /path/to/Turnip-v27.0.0-20261111-r2.zip
```

Output: `out/android-app/pacman-ce2.apk`. APK signing is checked by the script.
The APK includes SDL, runtime, driver and launcher resources, without game files.

## Prepare game and install

Prepare the game on the host first; see [the dump layout](../docs/building.md#supply-your-game):

```bash
python3 run.py prepare --game game/01PacManCE2
adb devices
bash tools/install_android.sh ADB_SERIAL out/game/boot-linked.bin game/01PacManCE2
adb -s ADB_SERIAL shell am start -n com.crownparkcomputing.pacmance2/.PacmanActivity
```

The installer uses `install -r`, stages the image and `content.bin`, copies game
files separately, and follows the selected game's shared module/trophy links.
For a runtime-only update, with game data already staged:

```bash
adb -s ADB_SERIAL install -r out/android-app/pacman-ce2.apk
```

External files are under
`/sdcard/Android/data/com.crownparkcomputing.pacmance2/files/`.
`pacman-ce2.log` is the main log; `launch.ini` accepts literal `TEMPEST_NAME=value`
lines. Normal play has no diagnostic timeout.

## Driver selection

Long-press the launcher icon and choose **Graphics driver**. Bundled Turnip is
the default; system-driver selection and importing another ARM64 driver ZIP are
available. Changes apply after closing and reopening the game. See
[driver settings and recovery](../docs/android-driver-selection.md).

## Defaults and checks

Android defaults to 1280×720 engine/display and authored effects, FEX MaxInst500,
single-sampled host images, complete decorative background and targeted completion
polling. It compiles shaders on demand each launch. Explicit `launch.ini` values
override defaults. Do not re-enable the older R5 driver for a speed comparison
without recording the driver change.

```bash
bash tools/run_android_fex.sh ADB_SERIAL
```

The real ARM64 execution check covers ABI calls, TLS, signal delivery, mapping
permissions and FEX call/return-stack guard recovery. GPU font captures match CPU
references. Three failed-level retries passed after the mutex signal-checkpoint
fix. Updating Turnip removes the measured multi-millisecond polling stalls; the
user confirms normal gameplay speed. Background restoration is visually verified;
the recorded hint-overlay frame rate is not a claim that every course sustains 60 FPS.

If returning from another app leaves a black surface, close and relaunch CE2.
One initial black startup also recovered after a cold restart; its cause remains
open. See [the status report](../docs/ce2-port-status.md) for measurements,
limitations and debugging history. The Android save is currently under
`files/user/savedata/1/UNKNOWN/AutoSave/`; Linux uses its own CUSA04924 profile.
Neither installation nor these build steps transfer or replace saves.

The internal menu is available with **L3 + R3 together**. Its **Graphics driver** row opens the same bundled/system/import-ZIP settings. Release both sticks before toggling again.
