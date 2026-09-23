#include "ui_field.h"
#include "field_tools.h"
#include "language.h"
#include "theme.h"
#include "settings.h"
#include <cstdio>
#include <cstring>
namespace FieldUI {
enum Page : uint8_t {
    HOME,
    EXTRA,
    PIT,
    DRONES,
    TELEMETRY,
    SENSORS,
    RULES,
    ACCESS,
    LANGUAGE,
    HELP,
    DISCOVER
};
static bool dirty = true;
static bool directEntry = false;
#if !defined(ARDUINO_ARCH_ESP32)
static uint32_t renders = 0;
uint32_t drawCount() {
    return renders;
}
#endif
static Page page = HOME;
static uint8_t selected = 0, slot = 0, help = 0, mutedSlot = 0;
static char status[64]{};
static uint8_t chosenMac[6]{};
static bool haveChosen = false;
void open() {
    directEntry = false;
    dirty = true;
    page = HOME;
    selected = slot = help = 0;
    status[0] = 0;
    Lang::resetTaps();
}
void openHelp() {
    open();
    page = HELP;
}
void openLanguage() { open(); page = LANGUAGE; directEntry = true; }
void openAccessibility() { open(); page = ACCESS; directEntry = true; }
void openRules() { open(); page = RULES; directEntry = true; }
bool needsDraw(uint32_t now, int width, int height) {
    static Field::Config previous;
    static uint32_t drawnAt = 0;
    static int oldW = 0, oldH = 0;
    bool changed =
        memcmp(&previous, &Field::config, sizeof previous) != 0 || oldW != width || oldH != height;
    bool live = page == DRONES || page == TELEMETRY || page == SENSORS || page == DISCOVER;
    if (!dirty && !changed && (!live || now - drawnAt < 250))
        return false;
    dirty = false;
    previous = Field::config;
    oldW = width;
    oldH = height;
    drawnAt = now;
    return true;
}
uint8_t currentPage() {
    return page;
}
static int rowY(int h, int i) {
    return 40 + i * ((h - 88) / 4);
}
static int rowH(int h) {
    return (h - 88) / 4 - 3;
}
static void line(TFT_eSPI &t, const char *text, int y, bool translate = true) {
    Lang::draw(t, text, 10, y, t.width() - 20, 18, Theme::WHITE, translate);
}
static void paragraph(TFT_eSPI &t, const char *text, int y, int h) {
    Lang::draw(t, text, 10, y, t.width() - 20, h, Theme::WHITE);
}
static void macText(char *b, size_t n, const uint8_t *m) {
    snprintf(b, n, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}
static void footer(TFT_eSPI &t, const char *right) {
    int w = t.width(), h = t.height();
    bool left = Field::config.left;
    Lang::button(t, left ? w / 2 + 2 : 8, h - 40, w / 2 - 10, 34, "BACK");
    Lang::button(t, left ? 8 : w / 2 + 2, h - 40, w / 2 - 10, 34, right);
}
static void row(TFT_eSPI &t, int i, const char *label, bool on = false) {
    Lang::button(t, 8, rowY(t.height(), i), t.width() - 16, rowH(t.height()), label, on);
}
static const char *helpText[] = {
    "Welcome. This device observes radio clues, not people or intent.",
    "Use Why this matched to inspect evidence. A possible match is not confirmation.",
    "No detection does not mean no camera.",
    "FPV equipment clues do not prove a drone is flying.",
    "Choose channels together before powering video transmitters. This board does not measure 5.8 "
    "GHz.",
    "Use microSD for exports. Raw research files can contain device identifiers.",
    "Verify a camera visually before reporting it. Open deflock.org/report on your phone.",
    "An upstream update replaces DNSP firmware. Reinstall a DNSP image from your-dnsp."};
void draw(TFT_eSPI &t, uint32_t now, const DetectionEngine &eng) {
#if !defined(ARDUINO_ARCH_ESP32)
    renders++;
#endif
    int w = t.width(), h = t.height();
    t.fillRect(0, 0, w, h, Theme::BG);
    t.setTextWrap(false);
    Theme::drawTitleBar(t, "FIELD TOOLS");
    const char *titles[] = {"FIELD TOOLS",   "FIELD TOOLS", "FPV PIT BOARD", "DRONE READINGS",
                            "OWN TELEMETRY", "MY SENSORS",  "ALERT RULES",   "ACCESSIBILITY",
                            "LANGUAGE",      "HELP",        "DISCOVER"};
    Lang::draw(t, titles[page], 32, 16, w - 64, 18, Theme::CYAN, true, true);
    char b[240]{};
    if (page == HOME) {
        row(t, 0, "FPV PIT BOARD");
        row(t, 1, "DRONE READINGS");
        row(t, 2, "OWN TELEMETRY");
        row(t, 3, "MY SENSORS");
    } else if (page == EXTRA) {
        row(t, 0, "ALERT RULES");
        row(t, 1, "ACCESSIBILITY");
        row(t, 2, "LANGUAGE");
        row(t, 3, "HELP");
    } else if (page == PIT) {
        for (int i = 0; i < 4; i++) {
            bool warn = false;
            for (int j = 0; j < 4; j++)
                warn |= Field::conflict(i, j);
            snprintf(b, sizeof b, "P%d  %c%u  %u MHz  %s", i + 1, "ABEFR"[Field::config.bands[i]],
                     Field::config.channels[i],
                     Field::frequency(Field::config.bands[i], Field::config.channels[i]),
                     warn ? "!" : "");
            row(t, i, b);
        }
    } else if (page == DRONES) {
        const auto &a = Field::aircraft(selected);
        snprintf(b, sizeof b, "%u/4  %s", selected + 1,
                 a.wifi ? "WiFi Remote ID" : "BLE Remote ID");
        line(t, b, 42, false);
        if (!a.used)
            paragraph(t, "NO DATA", 64, 54);
        else {
            snprintf(b, sizeof b, "ID: %.20s", a.info.haveBasic ? a.info.serial : "--");
            line(t, b, 64, false);
            if (a.info.haveLoc) {
                snprintf(b, sizeof b, "%.5f, %.5f", (double)a.info.lat, (double)a.info.lon);
                line(t, b, 84, false);
                snprintf(b, sizeof b, "Altitude %.0fm; age %lus", (double)a.info.altM,
                         (unsigned long)((now - a.info.locAt) / 1000));
                line(t, b, 104, false);
            }
            paragraph(t, RemoteId::qualityText(a.info, now), 126, h - 174);
        }
    } else if (page == TELEMETRY) {
        const auto &d = Field::telemetry();
        if (!Field::telemetryActive())
            paragraph(t, Field::telemetryStatus(), 42, h - 132);
        else {
            snprintf(b, sizeof b, "Heartbeat: %s",
                     d.heartbeat && (now - d.heartbeatAt) < 5000 ? "received" : "STALE / none");
            line(t, b, 42, false);
            if (d.battery) {
                snprintf(b, sizeof b, "%.2fV %d%% %s", d.millivolts / 1000., d.remaining,
                         now - d.batteryAt > 5000 ? "STALE" : "");
                line(t, b, 62, false);
            }
            if (d.position) {
                snprintf(b, sizeof b, "%.5f, %.5f", (double)d.lat, (double)d.lon);
                line(t, b, 82, false);
                snprintf(b, sizeof b, "Position age %lus",
                         (unsigned long)((now - d.positionAt) / 1000));
                line(t, b, 102, false);
            } else
                line(t, "NO DATA", 82);
            if (d.link) {
                snprintf(b, sizeof b, "Link %u / %u %s", d.rssi, d.remoteRssi,
                         now - d.linkAt > 5000 ? "STALE" : "");
                line(t, b, 122, false);
            }
        }
        Lang::button(t, 8, h - 84, w - 16, 36, Field::telemetryActive() ? "DISCONNECT" : "CONNECT");
    } else if (page == SENSORS) {
        const auto &s = Field::sensor(slot);
        snprintf(b, sizeof b, "Sensor slot %u/4", slot + 1);
        line(t, b, 42, false);
        if (Field::config.sensorOn[slot]) {
            macText(b, sizeof b, Field::config.sensors[slot]);
            line(t, b, 62, false);
        }
        if (s.used && Field::config.sensorOn[slot] &&
            !memcmp(s.mac, Field::config.sensors[slot], 6)) {
            snprintf(b, sizeof b, "%s  age %lus", now - s.at > 60000 ? "STALE" : "",
                     (unsigned long)((now - s.at) / 1000));
            line(t, b, 82, false);
            if (s.fields & 1) {
                snprintf(b, sizeof b, "Temp %.2f C %s", s.temperature / 100.,
                         s.trend ? (s.temperature > s.previous   ? "+"
                                    : s.temperature < s.previous ? "-"
                                                                 : "=")
                                 : "");
                line(t, b, 102, false);
            }
            if (s.fields & 2) {
                snprintf(b, sizeof b, "Humidity %.2f %%", s.humidity / 100.);
                line(t, b, 122, false);
            }
            if (s.fields & 4) {
                snprintf(b, sizeof b, "Battery %u %%", s.battery);
                line(t, b, 142, false);
            }
        } else
            line(t, "NO DATA", 86);
        Lang::button(t, 8, h - 80, w / 2 - 10, 34, "DISCOVER");
        Lang::button(t, w / 2 + 2, h - 80, w / 2 - 10, 34, "FORGET SENSOR");
    } else if (page == DISCOVER) {
        const auto &s = Field::discovery(selected);
        snprintf(b, sizeof b, "%u/4 -> slot %u", selected + 1, slot + 1);
        line(t, b, 42, false);
        if (s.used) {
            macText(b, sizeof b, s.mac);
            line(t, b, 66, false);
            snprintf(b, sizeof b, "Age %lus", (unsigned long)((now - s.at) / 1000));
            line(t, b, 88, false);
            Lang::button(t, 8, h - 84, w - 16, 36, "PIN SENSOR");
        } else
            line(t, "NO DATA", 66);
    } else if (page == RULES) {
        row(t, 0, "QUIET PREFIXES", Field::config.quietPrefix);
        row(t, 1, "COMPOSITE ONLY", Field::config.compositeOnly);
        row(t, 2, "MUTE SELECTED");
        row(t, 3, "CLEAR MUTES");
    } else if (page == ACCESS) {
        row(t, 0, "HIGH CONTRAST", Field::config.contrast);
        row(t, 1, "REDUCED MOTION", Field::config.reduced);
        row(t, 2, "LEFT HANDED", Field::config.left);
        row(t, 3, "LARGE CONTROLS", Field::config.large);
    } else if (page == LANGUAGE) {
        Lang::button(t, 8, 54, w - 16, 40, Lang::name(Field::config.language));
        paragraph(t, "Language preview. Translations need human review.", 108, h - 156);
    } else if (page == HELP) {
        snprintf(b, sizeof b, "%u/8", help + 1);
        line(t, b, 42, false);
        paragraph(t, helpText[help], 68, h - 116);
    }
    const char *right = page == PIT         ? "EXPORT TO SD"
                        : page == SENSORS   ? "NEXT"
                        : page == TELEMETRY ? "HELP"
                        : page == LANGUAGE  ? "HELP"
                                            : "NEXT";
    footer(t, right);
    if (status[0]) {
        char shortStatus[64];
        unsigned maxChars = (w - 16) / 6;
        snprintf(shortStatus, sizeof shortStatus, "%.*s", (int)maxChars, status);
        t.setTextFont(1);
        t.setTextSize(1);
        t.setTextColor(Theme::AMBER, Theme::BG);
        t.setCursor(8, h - 49);
        t.print(shortStatus);
    }
    (void)eng;
}
bool tap(int x, int y, int w, int h, uint32_t now, const DetectionEngine &eng) {
    if (x < 0 || x >= w || y < 0 || y >= h)
        return false;
    dirty = true;
    status[0] = 0;
    if (page == LANGUAGE && y >= 12 && y < 38 && x >= 32 && x < w - 32) {
        if (Lang::titleTap(now))
            strcpy(status, "Additional language available");
        return false;
    }
    if (y >= h - 40 && y < h - 6) {
        bool back = (x < w / 2) != bool(Field::config.left);
        if (back) {
            Field::telemetryStop();
            Lang::resetTaps();
            if (page == HOME || directEntry)
                return true;
            if (page == EXTRA) {
                page = HOME;
                return false;
            }
            page = HOME;
            return false;
        }
        if (page == HOME)
            page = EXTRA;
        else if (page == EXTRA)
            page = HOME;
        else if (page == PIT)
            strcpy(status,
                   Field::exportPit() ? "Saved /dnsp-fpv-pit.csv" : "microSD export failed");
        else if (page == SENSORS)
            slot = (slot + 1) % 4;
        else if (page == DRONES || page == DISCOVER)
            selected = (selected + 1) % 4;
        else if (page == TELEMETRY) {
            Field::telemetryStop();
            page = HELP;
            help = 3;
        } else if (page == LANGUAGE) {
            Lang::resetTaps();
            page = HELP;
            help = 0;
        } else if (page == HELP)
            help = (help + 1) % 8;
        else if (page == RULES) {
            selected = eng.logCount() ? (selected + 1) % eng.logCount() : 0;
            const Detection *d = eng.logAt(selected);
            haveChosen = d != nullptr;
            if (d) {
                memcpy(chosenMac, d->mac, 6);
                macText(status, sizeof status, d->mac);
            }
        }
        return false;
    }
    if (page == LANGUAGE && y >= 54 && y < 94) {
        Lang::next();
        return false;
    }
    if (page == TELEMETRY && y >= h - 84 && y < h - 48) {
        if (Field::telemetryActive())
            Field::telemetryStop();
        else
            Field::telemetryStart();
        return false;
    }
    if (page == SENSORS && y >= h - 80 && y < h - 46) {
        if (x < w / 2) {
            page = DISCOVER;
            selected = 0;
        } else {
            Field::config.sensorOn[slot] = false;
            Field::save();
        }
        return false;
    }
    if (page == DISCOVER && y >= h - 84 && y < h - 48) {
        const auto &s = Field::discovery(selected);
        if (s.used && now - s.at < 60000) {
            memcpy(Field::config.sensors[slot], s.mac, 6);
            Field::config.sensorOn[slot] = true;
            Field::save();
            page = SENSORS;
        }
        return false;
    }
    int r = -1;
    for (int i = 0; i < 4; i++)
        if (y >= rowY(h, i) && y < rowY(h, i) + rowH(h))
            r = i;
    if (r < 0)
        return false;
    if (page == HOME) {
        Page pages[] = {PIT, DRONES, TELEMETRY, SENSORS};
        page = pages[r];
        selected = 0;
    } else if (page == EXTRA) {
        Page pages[] = {RULES, ACCESS, LANGUAGE, HELP};
        page = pages[r];
        haveChosen = false;
        Lang::resetTaps();
    } else if (page == PIT) {
        if (x < w / 3)
            Field::config.bands[r] = (Field::config.bands[r] + 1) % 5;
        else
            Field::config.channels[r] = (Field::config.channels[r] + 1) % 9;
        Field::save();
    } else if (page == ACCESS) {
        uint8_t *options[] = {&Field::config.contrast, &Field::config.reduced, &Field::config.left,
                              &Field::config.large};
        *options[r] = !*options[r];
        Field::save();
        Theme::applyPalette(Settings::paletteIndex());
    } else if (page == RULES) {
        if (r == 0)
            Field::config.quietPrefix = !Field::config.quietPrefix;
        if (r == 1)
            Field::config.compositeOnly = !Field::config.compositeOnly;
        if (r == 2) {
            if (haveChosen) {
                memcpy(Field::config.muted[mutedSlot], chosenMac, 6);
                Field::config.muteOn[mutedSlot] = true;
                mutedSlot = (mutedSlot + 1) % 4;
                strcpy(status, "Selected device silenced; still logged");
            } else
                strcpy(status, "Tap NEXT to choose a logged device");
        }
        if (r == 3)
            for (auto &m : Field::config.muteOn)
                m = false;
        Field::save();
    }
    return false;
}
} // namespace FieldUI
