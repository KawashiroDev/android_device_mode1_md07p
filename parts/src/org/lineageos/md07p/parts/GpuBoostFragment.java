/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.md07p.parts;

import android.os.Bundle;
import android.os.SystemProperties;
import android.widget.CompoundButton;

import androidx.preference.CheckBoxPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import com.android.settingslib.widget.MainSwitchPreference;

public class GpuBoostFragment extends PreferenceFragmentCompat
        implements CompoundButton.OnCheckedChangeListener {

    // init.mt6789.rc sets the GPU devfreq max_freq from this property (1100 MHz when set,
    // otherwise the stock 1003 MHz), whenever it changes. It is not persistent.
    private static final String PROP_GPU_BOOST = "sys.md07p.gpu_boost";
    // Copied into PROP_GPU_BOOST at boot. Only left set when "Keep on after restart" is checked.
    private static final String PROP_GPU_BOOST_BOOT = "persist.sys.md07p.gpu_boost";
    private static final String PROP_GPU_BOOST_KEEP = "persist.sys.md07p.gpu_boost_keep";

    private CheckBoxPreference mKeep;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.gpu_boost, rootKey);

        // The properties, not SharedPreferences, are the source of truth.
        MainSwitchPreference boost = findPreference("gpu_boost");
        boost.setPersistent(false);
        boost.setChecked(isBoostOn());
        boost.addOnSwitchChangeListener(this);

        mKeep = findPreference("gpu_boost_keep");
        mKeep.setPersistent(false);
        // Before this option existed, GPU boost always stayed on after a restart.
        mKeep.setChecked(SystemProperties.getBoolean(PROP_GPU_BOOST_KEEP,
                SystemProperties.getBoolean(PROP_GPU_BOOST_BOOT, false)));
        mKeep.setOnPreferenceChangeListener(this::onKeepChanged);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        SystemProperties.set(PROP_GPU_BOOST, isChecked ? "1" : "0");
        saveBootState(isChecked, mKeep.isChecked());
    }

    private boolean onKeepChanged(Preference preference, Object newValue) {
        boolean keep = (Boolean) newValue;
        SystemProperties.set(PROP_GPU_BOOST_KEEP, keep ? "1" : "0");
        saveBootState(isBoostOn(), keep);
        return true;
    }

    private static boolean isBoostOn() {
        return SystemProperties.getBoolean(PROP_GPU_BOOST, false);
    }

    private static void saveBootState(boolean boostOn, boolean keep) {
        SystemProperties.set(PROP_GPU_BOOST_BOOT, boostOn && keep ? "1" : "0");
    }
}
