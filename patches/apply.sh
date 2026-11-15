#!/bin/bash
# Apply the MD-07P patches to other projects in the LineageOS tree. Run from the top of the tree
# after every `repo sync` (sync resets these projects). Safe to re-run: applied patches are skipped.
set -e
top=$(pwd)
here=$(dirname "$(readlink -f "$0")")
[ -d "$top/.repo" ] || { echo "run from the top of the LineageOS tree" >&2; exit 1; }

apply() {  # <project dir> <patch>
    cd "$top/$1"
    if git apply --check --reverse "$2" 2>/dev/null; then
        echo "already applied: $(basename "$2")"
    else
        git apply "$2" && echo "applied: $(basename "$2")"
    fi
}

apply packages/apps/FMRadio "$here/packages_apps_FMRadio-0001-band-from-props.patch"
apply frameworks/opt/telephony "$here/frameworks_opt_telephony-0001-trust-IMS-voice-over-Wi-Fi-for-service-state.patch"
apply prebuilts/openeuicc-deps "$here/prebuilts_openeuicc-deps-0001-build-against-sdk-current.patch"
apply bionic "$here/bionic-0001-posix_spawn-CLOEXEC_DEFAULT-fallback-for-pre-5.11-kernels.patch"
# OpenEUICC's submodules (lpac, cJSON) are not always fetched by repo sync-s
git -C "$top/packages/apps/OpenEUICC" submodule update --init --depth 1
