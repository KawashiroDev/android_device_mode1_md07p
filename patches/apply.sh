#!/bin/bash
# Apply the MD-07P patches to other projects in the LineageOS tree. Run from the top of the tree
# after every `repo sync` (sync resets these projects). Safe to re-run: applied patches are skipped.
#
#   apply.sh            apply all patches, then fetch OpenEUICC's submodules if needed
#   apply.sh --offline  apply all patches, never touch the network
#   apply.sh --revert   take all patches back out (leaves the projects clean for `repo sync`)
set -e
top=$(pwd)
here=$(dirname "$(readlink -f "$0")")
[ -d "$top/.repo" ] || { echo "run from the top of the LineageOS tree" >&2; exit 1; }

mode=apply
offline=
for arg in "$@"; do
    case "$arg" in
        --revert) mode=revert ;;
        --offline) offline=1 ;;
        *) echo "unknown option: $arg" >&2; exit 1 ;;
    esac
done

# <project dir> <patch>
patches=(
    "packages/apps/FMRadio packages_apps_FMRadio-0001-band-from-props.patch"
    "frameworks/opt/telephony frameworks_opt_telephony-0001-trust-IMS-voice-over-Wi-Fi-for-service-state.patch"
    "prebuilts/openeuicc-deps prebuilts_openeuicc-deps-0001-build-against-sdk-current.patch"
    "bionic bionic-0001-posix_spawn-CLOEXEC_DEFAULT-fallback-for-pre-5.11-kernels.patch"
    "frameworks/base frameworks_base-0001-optionally-keep-pointer-off-the-HWC-cursor-plane.patch"
    "packages/apps/OpenEUICC packages_apps_OpenEUICC-0001-dont-read-euiccinfo2-in-validity-check.patch"
    "external/wpa_supplicant_8 external_wpa_supplicant_8-0001-wpa-version-3-only-with-sae-offload.patch"
    "packages/apps/LineageParts packages_apps_LineageParts-0001-notification-light-breathing-switch.patch"
)

apply() {  # <project dir> <patch>
    cd "$top/$1"
    if git apply --check --reverse "$2" 2>/dev/null; then
        echo "already applied: $(basename "$2")"
    else
        git apply "$2" && echo "applied: $(basename "$2")"
    fi
}

revert() {  # <project dir> <patch>
    cd "$top/$1"
    if git apply --check --reverse "$2" 2>/dev/null; then
        git apply --reverse "$2" && echo "reverted: $(basename "$2")"
    elif git apply --check "$2" 2>/dev/null; then
        echo "not applied: $(basename "$2")"
    else
        echo "cannot revert $(basename "$2"): $1 has other local changes" >&2
        return 1
    fi
}

for entry in "${patches[@]}"; do
    read -r project patch <<<"$entry"
    "$mode" "$project" "$here/$patch"
done

[ "$mode" = apply ] || exit 0

# OpenEUICC's submodules (lpac, cJSON) are not always fetched by repo sync-s
if [ -z "$offline" ]; then
    git -C "$top/packages/apps/OpenEUICC" submodule update --init --depth 1
elif git -C "$top/packages/apps/OpenEUICC" submodule status | grep -q '^-'; then
    echo "OpenEUICC submodules are missing; run apply.sh without --offline once" >&2
    exit 1
fi
