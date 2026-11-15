#!/usr/bin/env bash
# Package the experimental Android runner; the user's game stays outside the APK.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
port=$PWD
driver=${1:?Usage: package_android.sh TURNIP_DRIVER_ZIP}
driver=$(realpath "$driver")
build=${TEMPEST_FEX_BUILD:-$port/out/android-fex-ndk}
sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}
ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
if [[ -z $ndk ]]; then
    mapfile -t installed < <(printf '%s\n' "$sdk"/ndk/*/source.properties | sort -V)
    for ((i=${#installed[@]}-1; i>=0; --i)); do
        if [[ -f ${installed[i]} ]]; then ndk=${installed[i]%/source.properties}; break; fi
    done
fi
mapfile -t build_tools < <(printf '%s\n' "$sdk"/build-tools/*/aapt2 | sort -V)
bt=${build_tools[-1]%/aapt2}
mapfile -t platforms < <(printf '%s\n' "$sdk"/platforms/android-*/android.jar | sort -V)
platform=${platforms[-1]}
sdl=${TEMPEST_ANDROID_SDL:-$(sed -n 's/^TEMPEST_ANDROID_SDL:PATH=//p' "$build/CMakeCache.txt")}
sdl_source=${TEMPEST_ANDROID_SDL_SOURCE:-$(dirname "$sdl")/sdl-src}
[[ -f $build/android-libs/libmain.so && -f $sdl/lib/libSDL3.so ]] || {
    echo 'Build the Android main target and set TEMPEST_ANDROID_SDL first.' >&2; exit 1;
}
[[ -d $sdl_source/android-project/app/src/main/java/org/libsdl/app ]] || {
    echo 'Set TEMPEST_ANDROID_SDL_SOURCE to the matching SDL3 source.' >&2; exit 1;
}
package=$port/out/android-app/package
rm -rf -- "$package"
mkdir -p "$package/classes" "$package/lib/arm64-v8a" "$package/assets"
python3 tools/stage_android_driver.py "$driver" "$package/assets"
javac -nowarn -source 17 -target 17 -classpath "$platform" -d "$package/classes" \
    "$sdl_source"/android-project/app/src/main/java/org/libsdl/app/*.java \
    android/java/com/crownparkcomputing/pacmance2/PacmanActivity.java
mapfile -t classes < <(rg --files "$package/classes" -g '*.class')
"$bt/d8" --release --min-api 31 --lib "$platform" --output "$package" "${classes[@]}"
cp "$build"/android-libs/*.so "$package/lib/arm64-v8a/"
cp "$sdl/lib/libSDL3.so" "$package/lib/arm64-v8a/"
cp "$ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" "$package/lib/arm64-v8a/"
"$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" --strip-unneeded "$package"/lib/arm64-v8a/*.so
mkdir -p "$package/resources"
cp -R android/res/. "$package/resources/"
cp res/pacman-ce2-icon.png "$package/resources/drawable/pacman_ce2_icon.png"
"$bt/aapt2" compile --dir "$package/resources" -o "$package/resources.zip"
"$bt/aapt2" link -I "$platform" --manifest "$port/android/AndroidManifest.xml" \
    --min-sdk-version 31 --target-sdk-version 33 -A "$package/assets" -o "$package/base.apk" "$package/resources.zip"
(cd "$package" && zip -q -r base.apk classes.dex lib)
"$bt/zipalign" -f -p 4 "$package/base.apk" "$package/aligned.apk"
keystore=$HOME/.android/debug.keystore
[[ -f $keystore ]] || { echo 'Create an Android debug keystore first.' >&2; exit 1; }
"$bt/apksigner" sign --ks "$keystore" --ks-pass pass:android --ks-key-alias androiddebugkey \
    --key-pass pass:android --out "$port/out/android-app/pacman-ce2-dev.apk" "$package/aligned.apk"
"$bt/apksigner" verify "$port/out/android-app/pacman-ce2-dev.apk"
echo "Built $port/out/android-app/pacman-ce2-dev.apk"
