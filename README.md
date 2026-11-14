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
| Modem | Boots, IMEI reads; calls/SMS/data **untested** (no SIM yet). VoLTE/IMS not working (see below) |
| eSIM | OpenEUICC sees the eUICC; adding a profile **untested** |
| Rear/front camera, photo and video | Works. Leave Aperture's video stabilisation off: MediaTek EIS makes video exposure step/flicker |
| FM radio, 76.0-108.0 MHz | Works; needs a wired headset as antenna |
| GPS | HAL and mnld run; outdoor fix **untested** |
| Fingerprint (MicroArray) | Works |
| Sensors, autorotate | Works; in-call proximity **untested** |
| Notification/battery LED | Works |
| Vibration (Awinic haptics) | Works |
| Charging control incl. bypass | Works. Status still reads "Charging" while paused (MediaTek driver) |

### Known issues

- **IMS / VoLTE**: MediaTek's `ImsService` expects stock's `mediatek-ims-*` and
  `mediatek-telephony-base` jars on the boot classpath and crash-loops without them. Until
  that is fixed, disable it after flashing: `adb shell pm disable com.mediatek.ims` (root).

## Building

```bash
repo init -u https://github.com/LineageOS/android.git -b lineage-23.2 --git-lfs
mkdir -p .repo/local_manifests
curl -o .repo/local_manifests/md07p.xml \
    https://raw.githubusercontent.com/YOUR_GITHUB_USER/android_device_mode1_md07p/md07p-bringup-fixes/manifest/md07p.xml
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
  `ro.fm.low_band`/`ro.fm.high_band` instead of a hard-coded 87.5 MHz floor.
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
