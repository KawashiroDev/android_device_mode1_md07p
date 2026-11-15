# LineageOS 23.2 device tree for the Mode1 Pocket (MD-07P)

MediaTek Helio G99 (MT6789), 720x1560 ILI9883C panel at up to 120 Hz, A/B with virtual A/B.

This tree started as 2by2's port of a Xiaomi tablet tree ("yunluo"); the
`md07p-bringup-fixes` branch replaces the tablet leftovers with the MD-07P's stock
configuration. SELinux is enforcing.

## Status

| Feature | Status |
| --- | --- |
| Display, touch, brightness, 60/90/120 Hz | Works |
| Wi-Fi, Bluetooth (incl. A2DP) | Works |
| Calls, SMS, mobile data | Works (tested on EE UK): earpiece, speakerphone, in-call proximity |
| VoLTE (HD calls), Wi-Fi calling | Works (EE UK), incl. Wi-Fi calls in airplane mode |
| Video calling (ViLTE) | Carrier-dependent; `vtservice` runs, but the modem's operator table disables video on EE, as on stock |
| eSIM | Works via OpenEUICC (profile downloaded and used for data) |
| Rear/front camera, photo and video | Works. Leave Aperture's video stabilisation off: MediaTek EIS makes video exposure step/flicker |
| FM radio, 64.0-108.0 MHz | Works; needs a wired headset as antenna |
| GPS | HAL and mnld run; outdoor fix **untested** |
| Fingerprint (MicroArray) | Works |
| Sensors, autorotate | Works |
| Notification/battery LED | Works |
| Vibration (Awinic haptics) | Works |
| Charging control incl. bypass | Works. Status still reads "Charging" while paused (MediaTek driver) |

### Known issues

- AOSP's carrier configs usually don't mark VoLTE or Wi-Fi calling as available (Google's
  CarrierSettings normally does). `overlays/CarrierConfigOverlay` turns both on for every
  carrier on this device; carriers without them simply reject IMS registration and calls
  use 2G/3G.

## Building

```bash
repo init -u https://github.com/LineageOS/android.git -b lineage-23.2 --git-lfs
mkdir -p .repo/local_manifests
curl -o .repo/local_manifests/md07p.xml \
    https://raw.githubusercontent.com/KawashiroDev/android_device_mode1_md07p/md07p-bringup-fixes/manifest/md07p.xml
repo sync -c -j8
device/mode1/md07p/patches/apply.sh   # after every repo sync

source build/envsetup.sh
lunch lineage_md07p bp4a userdebug    # note the space-separated form
m bacon
```

`manifest/md07p.xml` pulls the kernel prebuilts
(`android_device_mode1_md07p-kernel`) and proprietary blobs (`android_vendor_mode1_md07p`).
To regenerate the blobs from your own phone's firmware instead, point `extract-files.py` at
a directory laid out like the device's partitions (`system/`, `vendor/`, `system_ext/`,
`product/`, `odm/`...).

`patches/apply.sh` applies:

- `packages_apps_FMRadio-0001-band-from-props.patch`: FMRadio reads its band from
  `ro.fm.low_band`/`ro.fm.high_band` (app and JNI seek/scan) instead of hard-coded 87.5/76 MHz floors.
- `ImsService/` (applied by `extract-files.py`, not `apply.sh`): MediaTek's IMS app uses
  `mediatek-ims-base` as a shared library instead of expecting it on the boot classpath,
  and its runtime-registered receivers are marked exported.
- `frameworks_opt_telephony-0001-trust-IMS-voice-over-Wi-Fi-for-service-state.patch`: the
  MediaTek modem runs the Wi-Fi calling (ePDG) tunnel itself and reports no IWLAN registration,
  so Android ignored IMS over Wi-Fi when deciding whether there is voice service. With
  `ro.telephony.vowifi_voice_service=true`, IMS voice over Wi-Fi counts as service and is
  reported as an IWLAN registration (outgoing Wi-Fi calls in airplane mode, lock screen, and
  the "<carrier> Wi-Fi Calling" network name, falling back to the SIM's carrier name).
- `prebuilts_openeuicc-deps-0001-build-against-sdk-current.patch`: OpenEUICC's dependency
  modules target the next Android SDK; build them against `current`.

The build needs about 32 GB of RAM (more is better; Soong analysis is the peak).

## Installing

1. Unlock the bootloader and flash `vbmeta` with verification disabled (stock `vbmeta`,
   flags 3).
2. **Copy slot A's firmware to slot B first.** Stock MD-07P units can ship with empty slot B
   firmware partitions (`lk`, `tee`, `gz`, `scp`, `sspm`, `spmfw`, `mcupm`, `dpm`, `pi_img`,
   `md1img`, `dtbo`, `vbmeta*`). An OTA switches slots and does not carry firmware, so an empty
   slot B will not boot.
3. Flash the build's `vendor_boot.img` (contains Lineage Recovery) to the active slot, boot
   recovery, then *Apply update -> Apply from ADB* and `adb sideload` the zip.

If a sideload fails at postinstall with `Cannot ensure there are no overflow snapshots`,
stale snapshot state from an interrupted update is left in `/metadata/ota`; from recovery
with root adb, `mount /metadata && rm -rf /metadata/ota`, then retry. Do not touch
`/metadata/vold` (the data encryption key).
