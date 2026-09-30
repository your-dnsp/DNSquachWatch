#pragma once
#include "state.h"
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#endif

// Copies, never pointers into the history ring. Producers may run on either
// ESP32 core. Full queues retain the oldest unread alerts and count omissions.
template<uint8_t Capacity = 8> class DetectionQueue {
public:
    static_assert(Capacity > 0, "Queue must have storage");
    static constexpr uint8_t CAP = Capacity;
    static constexpr uint32_t MAX_AGE_MS = 120000;
    void push(const Detection& d) {
        lock();
        for (uint8_t i = 0; i < count_; ++i) {
            Detection& old = rows_[(head_ + i) % CAP];
            if (old.type == d.type && !memcmp(old.mac, d.mac, 6)) {
                Detection replacement = d;
                replacement.name[sizeof replacement.name - 1] = 0;
                if (old.conf > d.conf) {
                    replacement.conf = old.conf; replacement.evidence = old.evidence;
                    replacement.evidenceBits = old.evidenceBits; replacement.signature = old.signature; replacement.vendor = old.vendor;
                }
                old = replacement;
                unlock();
                return;
            }
        }
        if (count_ < CAP) rows_[(head_ + count_++) % CAP] = d;
        else if (dropped_ != UINT32_MAX) ++dropped_;
        unlock();
    }
    bool pop(Detection& out, uint32_t now) {
        lock();
        while (count_) {
            out = rows_[head_];
            head_ = (head_ + 1) % CAP;
            --count_;
            if ((int32_t)(now - out.lastSeen) <= (int32_t)MAX_AGE_MS) { unlock(); return true; }
        }
        unlock();
        return false;
    }
    template<class Keep> uint8_t retain(Keep keep, uint32_t now) {
        lock();
        uint8_t kept = 0;
        for (uint8_t i = 0; i < count_; ++i) {
            const Detection d = rows_[(head_ + i) % CAP];
            if ((int32_t)(now - d.lastSeen) <= (int32_t)MAX_AGE_MS && keep(d))
                rows_[(head_ + kept++) % CAP] = d;
        }
        count_ = kept;
        unlock();
        return kept;
    }
    uint32_t dropped() { lock(); const auto n = dropped_; unlock(); return n; }
    void clear() { lock(); count_ = head_ = 0; unlock(); }
private:
    Detection rows_[CAP]{};
    uint8_t head_ = 0, count_ = 0;
    uint32_t dropped_ = 0;
#if defined(ARDUINO_ARCH_ESP32)
    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    void lock() { portENTER_CRITICAL(&mux_); }
    void unlock() { portEXIT_CRITICAL(&mux_); }
#else
    void lock() {}
    void unlock() {}
#endif
};

using AlertQueue = DetectionQueue<8>;
