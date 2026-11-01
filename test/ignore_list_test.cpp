// The ignore list's NVS persistence.
//
// Reported bug: with exactly one device on the list, deleting it worked --
// the row disappeared, count() read zero -- and then the device was back
// after a reboot. The cause is that Preferences::putBytes returns early on
// a zero-length value without touching NVS, so writing the now-empty list
// was a silent no-op and the previous blob, still holding that one device,
// survived. With two or more devices the write is non-empty and everything
// behaves, which is why it went unnoticed.
//
// Nothing in the in-memory state can show this: count() is correct either
// way. The assertion has to be about what is on disk, so these tests reach
// past IgnoreList to the Preferences namespace underneath it and ask how
// many bytes the next boot would find.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
#include <Arduino.h>
#include <cstdlib>
#include <filesystem>

static const char* NS  = "ignore";
static const char* KEY = "dev";

// What begin() would read on the next boot.
static size_t bytesOnDisk() {
    Preferences p;
    p.begin(NS, false);
    return p.getBytesLength(KEY);
}

static const uint8_t A[6] = { 0xB4, 0x1E, 0x52, 0x01, 0x02, 0x03 };
static const uint8_t B[6] = { 0xAC, 0x9F, 0xC3, 0x0A, 0x0B, 0x0C };

int main() {
    // The shim only persists when it has somewhere to persist to.
    const char* dir = "ignore_test_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);

    suite("Removing one of several leaves the rest on disk");

    ck("adding the first device works", IgnoreList::add(A, DetectionType::FLOCK));
    ck("adding the second works",       IgnoreList::add(B, DetectionType::RING));
    ck("count is 2", IgnoreList::count() == 2);
    ck("two records on disk", bytesOnDisk() == 14);

    ck("removing the first reports success", IgnoreList::remove(A));
    ck("count is 1", IgnoreList::count() == 1);
    ck("the removed device is gone", !IgnoreList::contains(A));
    ck("the other one is still there", IgnoreList::contains(B));
    // The survivor backfills index 0, so this also covers the swap-with-last.
    ck("one record on disk", bytesOnDisk() == 7);
    ck("and it is the right one", IgnoreList::typeAt(0) == DetectionType::RING);

    suite("Removing the last one empties the stored list too");

    ck("removing the last reports success", IgnoreList::remove(B));
    ck("count is 0", IgnoreList::count() == 0);
    ck("nothing in memory", !IgnoreList::contains(B));
    // The bug: this used to be 7. The list looked empty until the reboot
    // read the stale blob back and the device returned.
    ck("nothing on disk either -- it must not come back on reboot",
       bytesOnDisk() == 0);

    suite("clear() empties an already-populated list");

    ck("re-add", IgnoreList::add(A, DetectionType::FLOCK));
    ck("on disk", bytesOnDisk() == 7);
    IgnoreList::clear();
    ck("count is 0 after clear", IgnoreList::count() == 0);
    ck("nothing on disk after clear", bytesOnDisk() == 0);

    suite("Timed snoozes keep per-device suppression separate from permanent Ignore");
    SimClock::virtualTime=true;SimClock::nowMs=1000;
    using D=IgnoreList::SnoozeDuration;
    const D durations[]={D::MIN_10,D::MIN_30,D::HOUR_1,D::HOUR_4};
    const uint32_t ms[]={600000,1800000,3600000,14400000};
    for(unsigned i=0;i<4;i++){
        IgnoreList::clear();const uint32_t start=SimClock::nowMs;
        ck("selected duration is accepted",IgnoreList::snooze(A,durations[i]));
        ck("remaining duration is exact",IgnoreList::snoozeRemaining(A)==ms[i]);
        ck("unrelated device remains unsilenced",!IgnoreList::silenced(B));
        ck("temporary snooze is absent from permanent list",!IgnoreList::contains(A)&&bytesOnDisk()==0);
        SimClock::nowMs=start+ms[i]-1;ck("still snoozed one ms before expiry",IgnoreList::snoozed(A));
        SimClock::nowMs++;ck("expires at exact boundary",!IgnoreList::silenced(A));
    }
    IgnoreList::clear();SimClock::nowMs=0xfffffff0u;
    ck("four-hour snooze accepted near rollover",IgnoreList::snooze(A,D::HOUR_4));
    SimClock::nowMs+=14400000-1;ck("rollover does not expire early",IgnoreList::snoozed(A));
    SimClock::nowMs++;ck("rollover expires at selected duration",!IgnoreList::snoozed(A));
    IgnoreList::clear();SimClock::nowMs=1000;IgnoreList::snooze(A);
    SimClock::nowMs+=26UL*24*60*60*1000;ck("long-stale timer cannot revive after half a millis cycle",!IgnoreList::snoozed(A));
    ck("null and invalid durations fail",!IgnoreList::snooze(nullptr,D::MIN_10)&&!IgnoreList::snooze(A,(D)255));
    IgnoreList::clear();SimClock::nowMs=1000;
    for(unsigned i=0;i<IgnoreList::SNOOZE_MAX;i++){uint8_t mac[6]={1,2,3,4,5,(uint8_t)i};ck("bounded table accepts each slot",IgnoreList::snooze(mac,D::MIN_10));}
    ck("full table explicitly refuses another address",!IgnoreList::snooze(A,D::MIN_10));
    uint8_t first[6]={1,2,3,4,5,0};ck("full table does not evict an existing timer",IgnoreList::snoozed(first));
    ck("existing timer can be extended in a full table",IgnoreList::snooze(first,D::HOUR_4));
    SimClock::nowMs+=600000;ck("expired entries are reclaimed",IgnoreList::snooze(A,D::MIN_30));
    ck("long-lived timer survives expired-slot compaction",IgnoreList::snoozed(first));
    ck("explicit unsnooze resumes this device",IgnoreList::unsnooze(A)&&!IgnoreList::snoozed(A));
    IgnoreList::snooze(A);ck("one-argument API defaults to ten minutes",IgnoreList::snoozeRemaining(A)==600000);
    ck("Ignore remains persistent independent of snooze",IgnoreList::add(A,DetectionType::FLOCK));
    SimClock::nowMs+=600000;ck("expired timer cannot undo permanent Ignore",IgnoreList::silenced(A)&&IgnoreList::contains(A)&&bytesOnDisk()==7);
    IgnoreList::snooze(B);IgnoreList::clear();ck("clear removes permanent and temporary state",!IgnoreList::silenced(A)&&!IgnoreList::snoozed(B)&&bytesOnDisk()==0);
    Preferences prefs;prefs.begin("ignore",false);ck("timed snoozes never create a second NVS list",!prefs.isKey("snz"));
    SimClock::virtualTime=false;
    std::filesystem::remove_all(dir);
    return report();
}

