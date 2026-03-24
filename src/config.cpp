#include "config.h"

String     Config::_ssid;
String     Config::_pass;
Preferences Config::_prefs;

void Config::load() {
    _prefs.begin(NVS_NAMESPACE, true); // read-only
    _ssid = _prefs.getString(NVS_KEY_SSID, DEFAULT_AP_SSID);
    _pass = _prefs.getString(NVS_KEY_PASS, DEFAULT_AP_PASS);
    _prefs.end();
}

void Config::save(const String& ssid, const String& pass) {
    _ssid = ssid;
    _pass = pass;
    _prefs.begin(NVS_NAMESPACE, false); // read-write
    _prefs.putString(NVS_KEY_SSID, ssid);
    _prefs.putString(NVS_KEY_PASS, pass);
    _prefs.end();
}

String Config::getSSID() { return _ssid; }
String Config::getPass()  { return _pass; }
