#include "glyph_decode.h"
#include "language.h"
#include "language_data.h"
#include "field_tools.h"
#include "theme.h"
#include <cstring>
#include <algorithm>
namespace Lang {
static uint8_t taps = 0;
static uint32_t lastTap = 0;
const char *name(uint8_t n) {
    static const char *names[] = {"English", "Español",  "Français", "Deutsch",
                                  "日本語",  "简体中文", "עברית"};
    return names[n < 7 ? n : 0];
}
const char *lookup(const char *s) {
    if (!s)
        return "";
    uint8_t n = Field::config.language;
    if (!n || n >= 7)
        return s;
    size_t lo = 0, hi = sizeof catalog / sizeof catalog[0];
    while (lo < hi) {
        size_t m = (lo + hi) / 2;
        int d = strcmp(catalog[m][0], s);
        if (d < 0)
            lo = m + 1;
        else
            hi = m;
    }
    if (lo < sizeof catalog / sizeof catalog[0] && !strcmp(s, catalog[lo][0]))
        return catalog[lo][n];
    size_t len = strlen(s);
    if (len > 4 && s[0] == '[' && s[len - 1] == ']') {
        const char *a = s + 1;
        while (*a == ' ')
            a++;
        size_t z = (s + len - 1) - a;
        while (z && a[z - 1] == ' ')
            z--;
        for (const auto &r : catalog)
            if (strlen(r[0]) == z && !strncmp(a, r[0], z))
                return r[n];
    }
    return s;
}
bool titleTap(uint32_t now) {
    if (now - lastTap > 3000)
        taps = 0;
    lastTap = now;
    if (++taps < 7)
        return false;
    taps = 0;
    Field::config.hebrew = true;
    Field::save();
    return true;
}
void resetTaps() {
    taps = 0;
    lastTap = 0;
}
void next() {
    auto &c = Field::config;
    c.language = (c.language + 1) % (c.hebrew ? 7 : 6);
    Field::save();
}
uint16_t nextCodepoint(const char *&p) {
    uint8_t a = (uint8_t)*p;
    if (!a)
        return 0;
    ++p;
    if (a < 128)
        return a;
    unsigned count = a >= 0xc2 && a <= 0xdf ? 1 : a >= 0xe0 && a <= 0xef ? 2 : 0;
    uint16_t u = count == 1 ? a & 31 : a & 15;
    if (!count)
        return '?';
    for (unsigned i = 0; i < count; i++) {
        uint8_t b = (uint8_t)*p;
        if ((b & 0xc0) != 0x80)
            return '?';
        u = (u << 6) | (b & 63);
        ++p;
    }
    if ((count == 2 && u < 0x800) || (u >= 0xd800 && u <= 0xdfff))
        return '?';
    return u;
}
static const Glyph &glyph(uint16_t cp) {
    size_t lo = 0, hi = sizeof glyphs / sizeof glyphs[0];
    while (lo < hi) {
        size_t m = (lo + hi) / 2;
        if (glyphs[m].cp < cp)
            lo = m + 1;
        else
            hi = m;
    }
    if (lo < sizeof glyphs / sizeof glyphs[0] && glyphs[lo].cp == cp)
        return glyphs[lo];
    return glyph('?');
}
int width(const char *p) {
    int n = 0;
    if (!p)
        return 0;
    while (*p)
        n += glyph(nextCodepoint(p)).width;
    return n;
}
static bool heb(uint16_t c) {
    return c >= 0x590 && c <= 0x5ff;
}
static bool ltr(uint16_t c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= 0xc0 && c <= 0x24f);
}
// Catalog Hebrew has no combining marks. Keep embedded Latin identifiers,
// URLs and number runs LTR. Untrusted radio text never passes through this path.
static void reorder(uint16_t *a, int n) {
    std::reverse(a, a + n);
    for (int i = 0; i < n;) {
        if (!ltr(a[i])) {
            if (a[i] == '(')
                a[i] = ')';
            else if (a[i] == ')')
                a[i] = '(';
            i++;
            continue;
        }
        int j = i + 1, last = i;
        while (j < n && (ltr(a[j]) || a[j] == ' ' || a[j] == '.' || a[j] == '/' || a[j] == '-' ||
                         a[j] == ':' || a[j] == '_')) {
            if (ltr(a[j]))
                last = j;
            j++;
        }
        j = last + 1;
        std::reverse(a + i, a + j);
        i = j;
    }
}
static void ink(TFT_eSPI &t, uint16_t cp, int x, int y, uint16_t color) {
    const auto &g = glyph(cp);
    uint8_t b[32];
    if (!GlyphDecode::decode(glyphBits, sizeof glyphBits,
                            Field::config.language == 4 ? g.jpOffset : g.offset, g.width, b)) return;
    int stride = g.width / 8;
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < g.width; c++)
            if (b[r * stride + c / 8] & (0x80 >> (c % 8)))
                t.drawPixel(x + c, y + r, color);
}
int draw(TFT_eSPI &t, const char *input, int x, int y, int w, int h, uint16_t color, bool translate,
         bool centered) {
    const char *p = translate ? lookup(input) : input;
    if (!p || w < 16 || h < 16)
        return 0;
    int startY = y;
    while (*p && y + 16 <= startY + h) {
        while (*p == ' ')
            p++;
        uint16_t line[96]{};
        int n = 0, pixels = 0, lastSpace = -1;
        const char *afterSpace = nullptr;
        while (*p && n < 95) {
            const char *before = p;
            uint16_t cp = nextCodepoint(p);
            if (cp == '\n')
                break;
            int advance = glyph(cp).width;
            if (pixels + advance > w) {
                p = before;
                if (lastSpace > 0) {
                    n = lastSpace;
                    p = afterSpace;
                }
                break;
            }
            line[n++] = cp;
            pixels += advance;
            if (cp == ' ') {
                lastSpace = n - 1;
                afterSpace = p;
            }
        }
        if (!n)
            break;
        while (n && line[n - 1] == ' ')
            --n;
        bool rtl = false;
        pixels = 0;
        for (int i = 0; i < n; i++) {
            rtl |= heb(line[i]);
            pixels += glyph(line[i]).width;
        }
        if (rtl)
            reorder(line, n);
        int xx = x + (centered ? (w - pixels) / 2 : rtl ? w - pixels : 0);
        for (int i = 0; i < n; i++) {
            ink(t, line[i], xx, y, color);
            xx += glyph(line[i]).width;
        }
        y += 18;
    }
    return y - startY;
}
void button(TFT_eSPI &t, int x, int y, int w, int h, const char *label, bool on) {
    uint16_t bg = on ? Theme::PURPLE : Theme::BG, fg = on ? Theme::labelOn(bg) : Theme::WHITE;
    t.fillRect(x, y, w, h, bg);
    t.drawRect(x, y, w, h, Theme::CYAN);
    const char *s = lookup(label);
    if (width(s) <= w - 8)
        draw(t, s, x + 4, y + (h - 16) / 2, w - 8, 16, fg, false, true);
    else
        draw(t, s, x + 4, y + 2, w - 8, h - 4, fg, false, true);
}
} // namespace Lang
namespace Lang {
const char *detectionNote(DetectionType t) {
    if (t == DetectionType::DRONE)
        return "Remote ID broadcasts received. Open Drone readings for individual records and "
               "freshness warnings.";
    if (t == DetectionType::FPV)
        return "FPV equipment clues do not prove a drone is flying.";
    if (t == DetectionType::FLOCK || t == DetectionType::AXON || t == DetectionType::META ||
        t == DetectionType::ALPR || t == DetectionType::CAMERA || t == DetectionType::RING)
        return "Possible camera-related equipment. Radio clues do not confirm the model, owner or "
               "recording status.";
    if (t == DetectionType::AIRTAG || t == DetectionType::SAMSUNG_TAG ||
        t == DetectionType::GOOGLE_TAG || t == DetectionType::TILE || t == DetectionType::IBEACON)
        return "Possible tracking or proximity device. This does not establish its owner or "
               "intent.";
    return "A radio pattern matched. Inspect the evidence; this does not prove wrongdoing.";
}
const char *why(const Detection &d) {
    switch (d.evidence) {
    case MatchEvidence::OUI:
        return "Shared manufacturer prefix. This does not confirm the device type.";
    case MatchEvidence::SSID:
    case MatchEvidence::BLE_NAME:
        return "A network name matched. Names can be changed or imitated.";
    case MatchEvidence::RESEARCH_COMPOSITE:
        return "Experimental combined clues matched. This does not confirm a particular device or "
               "recording.";
    case MatchEvidence::BLE_REMOTE_ID:
    case MatchEvidence::WIFI_REMOTE_ID:
        return "Broadcast claims; identity not authenticated";
    case MatchEvidence::BLE_COMPANY:
    case MatchEvidence::BLE_SERVICE:
        return "Bluetooth fields matched a listed rule. Other devices can share or imitate these "
               "fields.";
    case MatchEvidence::DEAUTH_BURST:
        return "Repeated WiFi deauthentication frames came from the same claimed transmitter. "
               "Source addresses can be spoofed; this does not by itself prove an attack.";
    default:
        return "A radio pattern matched. Inspect the evidence; this does not prove wrongdoing.";
    }
}
} // namespace Lang
