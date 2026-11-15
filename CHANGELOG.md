# Changelog

## Unreleased

- Per-game Android graphics-driver settings: bundled Turnip by default, system-driver choice, validated ZIP import and a launcher shortcut that works without the renderer.

- Initial PAC-MAN Championship Edition 2 Linux x86-64 and Android ARM64 port, based on the Tempest 4000 runtime and shadPS4 renderer.
- Correct display alias publication and packed R8 font detiling; readable Android menus and playable levels with music and sound effects.
- Faster FEX startup through one fresh host-mapping snapshot per validation request.
- Deferred-signal checkpoints in blocking mutex waits fix the reproduced failed-level Retry freeze.
- FEX call/return shadow-stack guard recovery, covered by real ARM64 guard-wrap checks.
- Verified Retroid 720p engine/effects profile and restored decorative background.
- Turnip driver with Mesa's KGSL zero-timeout fix; nonblocking completion polling removes the measured driver stall.
- Linux/Android build guides, standalone Android dependency setup, and source-only CI.

Inherited Tempest release history is preserved in [docs/legacy/tempest-changelog.md](docs/legacy/tempest-changelog.md).
