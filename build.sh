#!/bin/bash
set -e

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DIST_DIR="$ROOT_DIR/dist"
CORES="$(nproc --all)"
DEBUG=0

for arg in "$@"; do
    case "$arg" in
        --debug|-d)
            DEBUG=1
            ;;
        *)
            DIST_DIR="$arg"
            ;;
    esac
done

echo
echo "*** Compiling display-toolbox ***"
TITLE_ID="$(grep -oP '"title_id":\s*"0x\K(\w+)' "$ROOT_DIR/sysmodule/perms.json")"

pushd "$ROOT_DIR/sysmodule"

make -j$CORES
popd > /dev/null

mkdir -p "$DIST_DIR/atmosphere/contents/$TITLE_ID/flags"
cp -vf "$ROOT_DIR/sysmodule/out/display-toolbox.nsp" "$DIST_DIR/atmosphere/contents/$TITLE_ID/exefs.nsp"
>"$DIST_DIR/atmosphere/contents/$TITLE_ID/flags/boot2.flag"
cp -vf "$ROOT_DIR/sysmodule/toolbox.json" "$DIST_DIR/atmosphere/contents/$TITLE_ID/toolbox.json"

echo
echo "*** Compiling display-toolbox overlay ***"
pushd "$ROOT_DIR/overlay"
make -j$CORES
popd > /dev/null

mkdir -p "$DIST_DIR/switch/.overlays"
cp -vf "$ROOT_DIR/overlay/display-toolbox.ovl" "$DIST_DIR/switch/.overlays/display-toolbox.ovl"
echo
echo "*** Copying assets ***"
mkdir -p "$DIST_DIR/config/display-toolbox"
cp -vf "$ROOT_DIR/config.ini.template" "$DIST_DIR/config/display-toolbox/config.ini.template"

echo
