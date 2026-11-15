# PAC-MAN CE2 / Arcade Game Series native port

Development workspace for Linux x86-64 and Android ARM64 (Retroid Pocket Flip2).
Based on Jon's Tempest 4000 native runtime and shadPS4 Vulkan renderer. Linux reaches the menus and playable tutorial with active audio. Android has clear text, playable levels, music and sound effects. Startup improved from about 117 to 21 seconds. The mutex callback fix passes three failed-level retries. Updating Turnip to a build with Mesa's KGSL zero-timeout fix removes the blocking GPU status checks; the user confirms normal gameplay speed on the Retroid. Android uses 720p, and the temporarily suppressed decorative background is restored in the current build.

The supplied CUSA06906 v1.00 package contains PAC-MAN Championship Edition 2 (CUSA04924), Galaga, PAC-MAN and Dig Dug. Its Unity 5.3.6p6 player uses shipped Mono and AOT modules. The default development target is `game/01PacManCE2`.

Game files are private local inputs under ignored `game/`; APKs contain only the runner and dependencies. The original package and the working Tempest workspace are preserved.

Follow the [Linux build guide](docs/building.md) or [Android build guide](android/README.md). Build Linux with `bash build.sh`, then launch directly into CE2 with `./play-ce2.sh`. The launcher uses the verified 1080p profile. Its gamepad or keyboard controls follow the original game's menus. `./play-ce2.sh --timeout 60` runs a bounded session. Android uses its own package `com.crownparkcomputing.pacmance2`.

Current evidence and remaining Android work are recorded in [`docs/ce2-port-status.md`](docs/ce2-port-status.md).

Source is GPL-2.0-or-later. [Third-party notices](THIRD_PARTY_NOTICES.md) record the inherited runtime and dependency origins.

On Android, long-press the game icon → **Graphics driver** to select bundled Turnip (the default), the system driver, or import another driver ZIP. [Driver settings](docs/android-driver-selection.md) apply on the next launch and are separate for each game.
