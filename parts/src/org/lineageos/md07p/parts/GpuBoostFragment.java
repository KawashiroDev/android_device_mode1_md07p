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

public class GpuBoostFragment extends PreferenceFragmentCompat
        implements CompoundButton.OnCheckedChangeListener {

    // init.mt6789.rc sets the GPU devfreq max_freq from this property (1100 MHz when set,
    // otherwise the stock 1003 MHz), at boot and whenever it changes.
    private static final String PROP_GPU_BOOST = "persist.sys.md07p.gpu_boost";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.gpu_boost, rootKey);

        MainSwitchPreference boost = findPreference("gpu_boost");
        // The property, not SharedPreferences, is the source of truth.
        boost.setPersistent(false);
        boost.setChecked(SystemProperties.getBoolean(PROP_GPU_BOOST, false));
        boost.addOnSwitchChangeListener(this);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        SystemProperties.set(PROP_GPU_BOOST, isChecked ? "1" : "0");
    }
}
