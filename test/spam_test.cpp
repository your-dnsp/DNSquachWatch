// SquachWatch-CYD — the spam flood: when one type of tracker goes quiet.
//
// What this guards: a flood of fake tags, each from a fresh address, used to
// raise an alert per fake -- 91 in 22 minutes at a hacker convention. The
// rules below are the ones that are wrong by default if nobody writes them
// down: real tags (heard for a while, or heard again) are never evidence,
// strays spread over half an hour never add up to a flood, the flood gets
// exactly one alert, it ends on its own, and one type's flood never quiets
// another type.
#include "test_util.h"
#include "spam_watch.h"

static const uint8_t AIRTAG = 6, TILE = 12;

int main() {
    suite("Seven fakes are not a flood; the eighth is");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        bool tripped = false;
        for (int i = 0; i < 7; i++) { now += 5000; tripped |= w.noteVanish(AIRTAG, 1500, 1, now); }
        ck("seven short-lived addresses do not trip it", !tripped && !w.active(AIRTAG, now));
        now += 5000;
        ck("the eighth does", w.noteVanish(AIRTAG, 1500, 1, now));
        ck("and the type is quiet now", w.active(AIRTAG, now));
        ck("eight fakes counted", w.fakes(AIRTAG) == 8);
    }

    suite("A flood gets exactly one alert");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(AIRTAG, 800, 1, now); }
        ck("the first ask announces it", w.takeAnnounce(AIRTAG));
        ck("the second does not", !w.takeAnnounce(AIRTAG));
        for (int i = 0; i < 20; i++) { now += 3000; w.noteVanish(AIRTAG, 800, 1, now); }
        ck("nor does more of the same flood", !w.takeAnnounce(AIRTAG));
        ck("which keeps counting", w.fakes(AIRTAG) == 28);
    }

    suite("Real tags are never evidence");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        for (int i = 0; i < 30; i++) { now += 2000; w.noteVanish(AIRTAG, 30000, 1, now); }
        ck("thirty tags each heard for half a minute: nothing", !w.active(AIRTAG, now));
        for (int i = 0; i < 30; i++) { now += 2000; w.noteVanish(AIRTAG, 1000, 2, now); }
        ck("thirty that came back: nothing", !w.active(AIRTAG, now));
    }

    suite("Strays spread over half an hour are not a flood");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        for (int i = 0; i < 12; i++) { now += 4 * 60000; w.noteVanish(AIRTAG, 1000, 1, now); }
        ck("one every four minutes never adds up", !w.active(AIRTAG, now));
    }

    suite("It ends on its own, and the next flood is announced again");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(AIRTAG, 800, 1, now); }
        w.takeAnnounce(AIRTAG);
        ck("still quiet at five minutes", w.active(AIRTAG, now + SpamWatch::QUIET_MS));
        now += SpamWatch::QUIET_MS + 1;
        ck("over a moment later", !w.active(AIRTAG, now));
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(AIRTAG, 800, 1, now); }
        ck("a new flood trips it again", w.active(AIRTAG, now));
        ck("and gets its own alert", w.takeAnnounce(AIRTAG));
        ck("counted from scratch", w.fakes(AIRTAG) == 8);
    }

    suite("One type's flood never quiets another");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(AIRTAG, 800, 1, now); }
        ck("AirTags are quiet", w.active(AIRTAG, now));
        ck("Tiles are not", !w.active(TILE, now));
    }

    suite("A flood too fast to vanish trips on the count");
    {
        SpamWatch w;
        ck("39 new in a minute is a crowd", !w.noteBurst(AIRTAG, 39, 1000000));
        ck("40 is a flood", w.noteBurst(AIRTAG, 40, 1060000));
        ck("and is announced", w.takeAnnounce(AIRTAG));
    }

    suite("Out of range types are ignored");
    {
        SpamWatch w;
        ck("type 200", !w.noteVanish(200, 100, 1, 5000) && !w.active(200, 5000) && !w.takeAnnounce(200));
    }

    return report();
}
