// SquachWatch-CYD — the speaker, on the boards that have one.
//
// The two Freenove S3 boards carry an ES8311 codec on the touch controller's
// I2C bus and a small mono amplifier behind it, with a speaker plug on the
// edge. Nothing else in the fleet has a speaker; this header is a set of
// no-ops everywhere else, so the pets and the mascot can ask for a sound
// without knowing whether anything is listening.
//
// Every sound is a short run of 8-bit style tones, picked from the Sound Lab
// artifact on 2026-10-11, and rendered on the fly: no samples in flash. The
// SOUND row in Settings is OFF (the default), ALERTS or ALL; VOLUME is LOW,
// MED or HIGH. The rules for when a sound may play are the CrowPanel
// buzzer's, applied to all of them: never with the screen dimmed, never at
// night (except a watch-list hit, which is what a watch list is for), never
// at boot, never through the security wipe. The alert sounds also follow the
// alert gate in main.cpp (alertMayInterrupt): a device the log never held.
#pragma once
#include <stdint.h>

#if defined(FREENOVE_S3) || defined(FREENOVE_S3_35)
#define SQW_SPEAKER 1
#else
#define SQW_SPEAKER 0
#endif

namespace Sound {
    enum class Id : uint8_t {
        NEW_DEVICE = 0,   // ALERTS: three climbing blips
        WATCH_HIT,        // ALERTS: the two-tone siren, three times
        LOW_BATTERY,      // ALERTS: two tired notes, once per dip under 10%
        MESSAGE,          // ALL: ding dong, a squad message
        VISITOR,          // ALL: yoo-hoo, a visitor walks in
        THROW,            // ALL: wheee, Squachy or a pet in the air
        LAND,             // ALL: splat, the first bounce off the floor
        CLIPPY,           // ALL: the paperclip boing, when C1iPPY speaks
        TOAST_POP,        // ALL: ptoing, a slice pops
        PLUG_IN,          // ALL: power up, the lamp sees the cable
        UNPLUG,           // ALL: the drain slide, the cable comes out
        COUNT
    };

#if SQW_SPEAKER
    // Once in setup(), after the touch controller has started the I2C bus.
    // Finds the codec, sets it up for 16 kHz playback and parks the amplifier
    // off. Boots silent: nothing plays until something asks.
    void begin();
    // Ask for a sound. Returns at once; a small task renders and plays it.
    // Honours the SOUND setting, night and the hush below unless `sample` is
    // set, which is how the Settings rows let the owner hear each level.
    void play(Id id, bool sample = false);
    // The screen is dimmed by the saver: whoever let it dim wanted the board
    // quiet. Nothing plays while this is set.
    void hush(bool on);
    // The security wipe: the amplifier off and the queue dropped, now.
    void quiet();
    // Whether the codec answered at begin(); the SYSTEM page reads it.
    bool present();
    // The bench: flip the amplifier enable's polarity if the schematic lied.
    void ampLow(bool activeLow);
#else
    inline void begin() {}
    inline void play(Id, bool = false) {}
    inline void hush(bool) {}
    inline void quiet() {}
    inline bool present() { return false; }
    inline void ampLow(bool) {}
#endif
}
