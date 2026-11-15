# PAC-MAN CE2 / Arcade Game Series native port

Target Linux x86-64 and Android ARM64 Retroid Pocket Flip2. The selected game is PAC-MAN CE2 (CUSA04924) from the CUSA06906 collection. Local game inputs belong under ignored game/; build outputs belong under ignored out/.

The runtime is based on Jon's working Tempest 4000 Linux/Android port, including its current uncommitted fixes. Keep Tempest and its installed Android package separate. Do not copy game-specific address patches into this game without checking its executable. Game assets stay under ignored game/ and build products under ignored out/. Never put game files into Git or an APK.

Use focused builds, runtime checks, and meaningful sequencing/timing tests. Do not add tests for simple branding or layout changes. Do not claim gameplay until observed.
