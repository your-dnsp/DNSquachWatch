// SquachWatch-CYD — the PIN lock. See include/security.h.
#include "security.h"
#include "dnsp_sha256.h"
#include "field_tools.h"
#include "ignore_list.h"
#include <Preferences.h>
#include <esp_system.h>   // esp_random(), as meshtalk.cpp uses it
#include <string.h>

namespace {

// ---- a self-contained SHA-256 -------------------------------------------
// So this file needs no crypto library and compiles the same on the device,
// the emulator and the host test. A PIN's whole space is brute-forceable
// offline whatever the hash, so this exists only to keep the digits from
// sitting in flash in the clear, not to resist an attacker with the salt.
using namespace DnspHash;
void hashPin(const uint8_t salt[8], const char* digits, uint8_t out[32]) {
    Sha256 c;
    shaInit(c);
    shaUpdate(c, salt, 8);
    shaUpdate(c, (const uint8_t*)digits, strlen(digits));
    shaFinal(c, out);
}

// ---- state --------------------------------------------------------------
const char* NS = "security";
Preferences s_prefs;

bool     s_enabled   = false;
uint8_t  s_len       = 4;
uint8_t  s_salt[8]   = { 0 };
uint8_t  s_hash[32]  = { 0 };
bool     s_hasDuress = false;
uint8_t  s_dsalt[8]  = { 0 };
uint8_t  s_dhash[32] = { 0 };
bool     s_lockAtBoot = false;
uint8_t  s_autoLock  = (uint8_t)Security::AutoLock::OFF;
bool     s_wipeOnFail = false;
uint8_t  s_lockAlerts = (uint8_t)Security::LockAlerts::TYPE_ONLY;

bool     s_locked    = false;
uint8_t  s_fails     = 0;
uint32_t s_waitStart = 0;      // millis() the current backoff began; RAM only

const uint8_t  FAIL_FREE   = 5;    // this many misses before any wait
const uint32_t FAIL_BASE   = 30000;
const uint32_t FAIL_MAX    = 30UL * 60UL * 1000UL;   // half an hour, capped
const uint8_t  WIPE_AT     = 10;

void freshSalt(uint8_t out[8]) {
    for (int i = 0; i < 8; i += 4) {
        uint32_t r = esp_random();
        out[i] = (uint8_t)r; out[i+1] = (uint8_t)(r>>8);
        out[i+2] = (uint8_t)(r>>16); out[i+3] = (uint8_t)(r>>24);
    }
}

uint32_t requiredWaitMs() {
    if (s_fails < FAIL_FREE) return 0;
    uint32_t w = FAIL_BASE;
    for (uint8_t i = FAIL_FREE; i < s_fails && w < FAIL_MAX; i++) w <<= 1;
    return w > FAIL_MAX ? FAIL_MAX : w;
}

void clearNamespace(const char* ns) {
    Preferences p;
    p.begin(ns, false);
    p.clear();
    p.end();
}

}  // namespace

namespace Security {

void begin() {
    s_prefs.begin(NS, false);
    s_locked = false;          // decided afresh below, from what was stored
    s_enabled = s_prefs.getBool("on", false);
    s_len     = s_prefs.getUChar("len", 4);
    if (s_len != 4 && s_len != 6 && s_len != 8) s_len = 4;
    if (s_prefs.getBytes("salt", s_salt, 8) != 8) s_enabled = false;
    if (s_prefs.getBytes("hash", s_hash, 32) != 32) s_enabled = false;
    s_hasDuress = s_prefs.getBool("don", false) &&
                  s_prefs.getBytes("dsalt", s_dsalt, 8) == 8 &&
                  s_prefs.getBytes("dhash", s_dhash, 32) == 32;
    s_lockAtBoot = s_prefs.getBool("boot", false);
    s_autoLock   = s_prefs.getUChar("auto", (uint8_t)AutoLock::OFF);
    if (s_autoLock > (uint8_t)AutoLock::MIN_30) s_autoLock = (uint8_t)AutoLock::OFF;
    s_wipeOnFail = s_prefs.getBool("wipe", false);
    s_lockAlerts = s_prefs.getUChar("alerts", (uint8_t)LockAlerts::TYPE_ONLY);
    if (s_lockAlerts > (uint8_t)LockAlerts::NONE) s_lockAlerts = (uint8_t)LockAlerts::TYPE_ONLY;
    s_fails      = s_prefs.getUChar("fails", 0);

    // Lock at boot when asked; and a device that was already in backoff when it
    // lost power comes back locked and still waiting, timed from now -- the
    // count survived, so pulling the plug bought nothing.
    if (s_enabled && (s_lockAtBoot || s_fails >= FAIL_FREE)) s_locked = true;
    s_waitStart = 0;   // set on the next real millis() in lockoutRemainingMs()
}

bool    enabled()   { return s_enabled; }
uint8_t pinLength() { return s_len; }
PinLen  pinLen()    { return (PinLen)s_len; }
bool    locked()    { return s_enabled && s_locked; }
void    lock()      { if (s_enabled) s_locked = true; }

void setPinLength(PinLen n) {
    if (s_enabled) return;                 // length is fixed once a PIN exists
    s_len = (uint8_t)n;
    s_prefs.putUChar("len", s_len);
}

bool setPin(const char* digits) {
    if (!digits || strlen(digits) != s_len) return false;
    freshSalt(s_salt);
    hashPin(s_salt, digits, s_hash);
    s_prefs.putBytes("salt", s_salt, 8);
    s_prefs.putBytes("hash", s_hash, 32);
    s_prefs.putBool("on", true);
    s_enabled = true;
    s_fails = 0; s_prefs.putUChar("fails", 0);
    return true;
}

void disable() {
    s_prefs.remove("on"); s_prefs.remove("salt"); s_prefs.remove("hash");
    s_prefs.remove("don"); s_prefs.remove("dsalt"); s_prefs.remove("dhash");
    s_prefs.remove("fails");
    s_enabled = false; s_hasDuress = false; s_locked = false; s_fails = 0;
}

bool hasDuress() { return s_hasDuress; }

bool setDuress(const char* digits) {
    if (!s_enabled || !digits || strlen(digits) != s_len) return false;
    // Must not be the real PIN -- otherwise the everyday unlock wipes.
    uint8_t h[32];
    hashPin(s_salt, digits, h);
    if (memcmp(h, s_hash, 32) == 0) return false;
    freshSalt(s_dsalt);
    hashPin(s_dsalt, digits, s_dhash);
    s_prefs.putBytes("dsalt", s_dsalt, 8);
    s_prefs.putBytes("dhash", s_dhash, 32);
    s_prefs.putBool("don", true);
    s_hasDuress = true;
    return true;
}

void clearDuress() {
    s_prefs.remove("don"); s_prefs.remove("dsalt"); s_prefs.remove("dhash");
    s_hasDuress = false;
}

bool lockAtBoot()        { return s_lockAtBoot; }
void setLockAtBoot(bool v) { s_lockAtBoot = v; s_prefs.putBool("boot", v); }

AutoLock autoLock() { return (AutoLock)s_autoLock; }
void cycleAutoLock() {
    s_autoLock = (uint8_t)((s_autoLock + 1) % ((uint8_t)AutoLock::MIN_30 + 1));
    s_prefs.putUChar("auto", s_autoLock);
}
const char* autoLockLabel() {
    switch ((AutoLock)s_autoLock) {
        case AutoLock::OFF:      return "OFF";
        case AutoLock::ON_SLEEP: return "ON SLEEP";
        case AutoLock::MIN_1:    return "1 MIN";
        case AutoLock::MIN_5:    return "5 MIN";
        case AutoLock::MIN_15:   return "15 MIN";
        case AutoLock::MIN_30:   return "30 MIN";
    }
    return "OFF";
}
uint32_t autoLockIdleMs() {
    switch ((AutoLock)s_autoLock) {
        case AutoLock::MIN_1:  return 60UL * 1000UL;
        case AutoLock::MIN_5:  return 5UL * 60UL * 1000UL;
        case AutoLock::MIN_15: return 15UL * 60UL * 1000UL;
        case AutoLock::MIN_30: return 30UL * 60UL * 1000UL;
        default: return 0;
    }
}
bool autoLockOnSleep() { return (AutoLock)s_autoLock == AutoLock::ON_SLEEP; }

bool wipeOnFail()        { return s_wipeOnFail; }
void setWipeOnFail(bool v) { s_wipeOnFail = v; s_prefs.putBool("wipe", v); }

LockAlerts lockAlerts() { return (LockAlerts)s_lockAlerts; }
void cycleLockAlerts() {
    s_lockAlerts = (uint8_t)((s_lockAlerts + 1) % ((uint8_t)LockAlerts::NONE + 1));
    s_prefs.putUChar("alerts", s_lockAlerts);
}
const char* lockAlertsLabel() {
    switch ((LockAlerts)s_lockAlerts) {
        case LockAlerts::FULL:      return "FULL";
        case LockAlerts::TYPE_ONLY: return "TYPE ONLY";
        case LockAlerts::NONE:      return "NONE";
    }
    return "TYPE ONLY";
}

void wipeSecrets() {
    Field::wipePrivate();
    clearNamespace("meshtalk");
    clearNamespace("otawifi");       // the saved WiFi password for updates
    IgnoreList::clear();            // empties RAM and its NVS blob
}

Check check(const char* digits, uint32_t now) {
    if (!s_enabled || !digits) return Check::WRONG;
    if (lockoutRemainingMs(now) > 0) return Check::WRONG;

    if (s_hasDuress) {
        uint8_t h[32];
        hashPin(s_dsalt, digits, h);
        if (memcmp(h, s_dhash, 32) == 0) {
            // Wipe, then behave exactly like a correct unlock: no message, no
            // delay. Whoever forced this sees an ordinary, empty SquachWatch.
            wipeSecrets();
            clearDuress();
            s_fails = 0; s_prefs.putUChar("fails", 0);
            s_locked = false;
            return Check::DURESS;
        }
    }

    uint8_t h[32];
    hashPin(s_salt, digits, h);
    if (memcmp(h, s_hash, 32) == 0) {
        s_fails = 0; s_prefs.putUChar("fails", 0);
        s_locked = false;
        return Check::OK;
    }

    // Wrong. Count it (persisted, so a reboot does not reset the backoff), then
    // start the wait and, at the tenth, wipe if that was asked for.
    if (s_fails < 255) s_fails++;
    s_prefs.putUChar("fails", s_fails);
    s_waitStart = now ? now : 1;
    if (s_wipeOnFail && s_fails >= WIPE_AT) {
        wipeSecrets();
        // Still locked -- guessing ten times is not a way in -- but with the
        // count cleared, or the owner would come back to a half-hour wait for
        // a device that no longer holds anything worth waiting for.
        s_fails = 0; s_prefs.putUChar("fails", 0);
        return Check::WIPED;
    }
    return Check::WRONG;
}

uint32_t lockoutRemainingMs(uint32_t now) {
    const uint32_t need = requiredWaitMs();
    if (!need) return 0;
    // First call after a boot that came up already in backoff: start the clock
    // now, so the wait is served from boot rather than skipped.
    if (s_waitStart == 0) s_waitStart = now ? now : 1;
    const uint32_t waited = now - s_waitStart;
    return waited >= need ? 0 : need - waited;
}

bool verify(const char* digits) {
    if (!s_enabled || !digits || strlen(digits) != s_len) return false;
    uint8_t h[32];
    hashPin(s_salt, digits, h);
    return memcmp(h, s_hash, 32) == 0;
}

bool isDuress(const char* digits) {
    if (!s_hasDuress || !digits || strlen(digits) != s_len) return false;
    uint8_t h[32];
    hashPin(s_dsalt, digits, h);
    return memcmp(h, s_dhash, 32) == 0;
}

void forceUnlock() { s_locked = false; }

uint8_t failCount() { return s_fails; }

}  // namespace Security

