/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.md07p.parts;

import android.os.Bundle;
import android.os.SystemProperties;
import android.widget.CompoundButton;

import androidx.preference.PreferenceFragmentCompat;

import com.android.settingslib.widget.MainSwitchPreference;

public class BrightnessBoostFragment extends PreferenceFragmentCompat
        implements CompoundButton.OnCheckedChangeListener {

    // Read by the vendor lights HAL (android.hardware.light-service.md07p), which applies
    // the change immediately.
    private static final String PROP_BRIGHTNESS_BOOST = "persist.sys.md07p.brightness_boost";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.brightness_boost, rootKey);

        MainSwitchPreference boost = findPreference("brightness_boost");
        // The property, not SharedPreferences, is the source of truth.
        boost.setPersistent(false);
        boost.setChecked(SystemProperties.getBoolean(PROP_BRIGHTNESS_BOOST, false));
        boost.addOnSwitchChangeListener(this);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        SystemProperties.set(PROP_BRIGHTNESS_BOOST, isChecked ? "1" : "0");
    }
}
