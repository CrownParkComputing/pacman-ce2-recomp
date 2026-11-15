#!/usr/bin/env bash
# Install the development APK and stage the user's own game separately.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
device=${1:?Usage: install_android.sh ADB_SERIAL BOOT_LINKED_BIN GAME_DIR}
image=${2:?Usage: install_android.sh ADB_SERIAL BOOT_LINKED_BIN GAME_DIR}
game=${3:?Usage: install_android.sh ADB_SERIAL BOOT_LINKED_BIN GAME_DIR}
[[ -s $image && -s $game/eboot.bin ]] || { echo 'Need a nonempty prepared image and decrypted PS4 game executable.' >&2; exit 1; }
profile=$(dirname -- "$image")/content.bin
[[ -s $profile && -d $game/sce_module ]] || { echo 'Need content.bin beside the image and the selected game’s shared sce_module directory.' >&2; exit 1; }
sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}
adb=${ADB:-$sdk/platform-tools/adb}
[[ -x $adb ]] || adb=$(command -v adb)
package=com.crownparkcomputing.pacmance2
remote=/sdcard/Android/data/$package/files
"$adb" -s "$device" install -r out/android-app/pacman-ce2-dev.apk
"$adb" -s "$device" shell mkdir -p "$remote/app0" "$remote/user"
"$adb" -s "$device" push "$image" "$remote/boot-linked.bin"
"$adb" -s "$device" push "$profile" "$remote/content.bin"
for entry in "$game"/*; do
    [[ ${entry##*/} == sce_module ]] && continue
    if [[ ${entry##*/} == sce_sys && -L $entry/trophy ]]; then
        # The collection shares its trophy packages at the outer game root.
        # Stage that directory explicitly instead of relying on adb symlinks.
        "$adb" -s "$device" shell mkdir -p "$remote/app0/sce_sys"
        for metadata in "$entry"/*; do
            [[ ${metadata##*/} == trophy ]] && continue
            "$adb" -s "$device" push "$metadata" "$remote/app0/sce_sys/"
        done
        continue
    fi
    "$adb" -s "$device" push "$entry" "$remote/app0/"
done
if [[ -d $game/sce_sys/trophy ]]; then
    "$adb" -s "$device" shell mkdir -p "$remote/app0/sce_sys/trophy"
    "$adb" -s "$device" push "$(realpath "$game/sce_sys/trophy")/." "$remote/app0/sce_sys/trophy/"
fi
"$adb" -s "$device" shell mkdir -p "$remote/app0/sce_module"
"$adb" -s "$device" push "$(realpath "$game/sce_module")/." "$remote/app0/sce_module/"
echo "Installed development build and staged game for $package"
