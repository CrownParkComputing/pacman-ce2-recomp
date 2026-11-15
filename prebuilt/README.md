# Minimal FFmpeg Linux archive

`ffmpeg-linux.tar.xz` is the inherited minimal static FFmpeg 9.0.2 build used by
`tools/build_ffmpeg.sh`. FFmpeg is LGPL-2.1-or-later for this configuration;
see [the licence](FFmpeg-COPYING.LGPLv2.1).

Corresponding upstream source: https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz

Source SHA-256: `8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e`.
The complete configuration and rebuild steps are in `tools/build_ffmpeg.sh` and
`tools/pack_ffmpeg_linux.sh`. Set `TEMPEST_FFMPEG_PREBUILT=0` to compile source.
No game or proprietary codec modules are in this archive.
