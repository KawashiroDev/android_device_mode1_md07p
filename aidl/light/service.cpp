/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Lights.h"

#define LOG_TAG "android.hardware.light-service.md07p"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <sys/system_properties.h>

#include <functional>
#include <thread>

using ::aidl::android::hardware::light::Lights;

static constexpr const char* kBrightnessBoostProp = "persist.sys.md07p.brightness_boost";
static constexpr const char* kLedBreatheProp = "persist.sys.lineage.led_breathe";

// Follow a Settings toggle: apply the current value, then re-apply on every change.
static void watchBoolProp(const char* name, std::function<void(bool)> apply) {
    // Persistent properties only exist once /data is up (and once first set); until then
    // the default (off) stays.
    android::base::WaitForPropertyCreation(name);
    const prop_info* pi = __system_property_find(name);
    if (pi == nullptr) {
        LOG(ERROR) << "Cannot find " << name;
        return;
    }

    uint32_t serial = 0;
    while (true) {
        apply(android::base::GetBoolProperty(name, false));
        if (!__system_property_wait(pi, serial, &serial, nullptr)) {
            LOG(ERROR) << "Waiting for " << name << " failed";
            return;
        }
    }
}

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Lights> lights = ndk::SharedRefBase::make<Lights>();

    const std::string instance = std::string() + Lights::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(lights->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    std::thread(watchBoolProp, kBrightnessBoostProp,
                [lights](bool on) { lights->setBrightnessBoost(on); })
            .detach();
    std::thread(watchBoolProp, kLedBreatheProp, [lights](bool on) { lights->setBreathe(on); })
            .detach();

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
