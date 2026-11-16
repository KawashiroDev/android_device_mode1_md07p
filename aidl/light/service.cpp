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

#include <thread>

using ::aidl::android::hardware::light::Lights;

static constexpr const char* kBrightnessBoostProp = "persist.sys.md07p.brightness_boost";

// Follow the Settings toggle: apply the current value, then re-apply on every change.
static void watchBrightnessBoost(std::shared_ptr<Lights> lights) {
    // Persistent properties only exist once /data is up; until then the cap stays on.
    android::base::WaitForPropertyCreation(kBrightnessBoostProp);
    const prop_info* pi = __system_property_find(kBrightnessBoostProp);
    if (pi == nullptr) {
        LOG(ERROR) << "Cannot find " << kBrightnessBoostProp;
        return;
    }

    uint32_t serial = 0;
    while (true) {
        lights->setBrightnessBoost(android::base::GetBoolProperty(kBrightnessBoostProp, false));
        if (!__system_property_wait(pi, serial, &serial, nullptr)) {
            LOG(ERROR) << "Waiting for " << kBrightnessBoostProp << " failed";
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

    std::thread(watchBrightnessBoost, lights).detach();

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
