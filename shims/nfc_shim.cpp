/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * The stock SLM NFC HAL (nfc_nci_slm.so) was linked against Android 15's libbase, which had a
 * non-template android::base::Trim(const std::string&). Android 16 made Trim a template, so the
 * old symbol is gone. Provide it here.
 */

#include <cctype>
#include <string>

namespace android {
namespace base {

std::string Trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

}  // namespace base
}  // namespace android
