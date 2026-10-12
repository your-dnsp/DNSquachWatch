// th3_0n3.inc -- TH3 0N3, outfit 18.
//
// A long near-black coat with violet lapels, his own chunky shades made
// round with the code falling inside them, and a handful of things he does:
//   - the code in his lenses runs red while something serious is nearby
//     (setNearbyLit, from the main screen),
//   - every few seconds the lenses glitch pink and one tears sideways,
//   - bullet time every fourteen seconds: the lens code freezes, rings of air
//     open out behind him and a bullet creeps past his head; tap him while
//     it crosses and he ducks it,
//   - tap a lens for a red pill or a blue one,
//   - pick him up and he bends a spoon,
//   - now and then the same black cat walks past him twice,
//   - the payphone rings for him (ui_phone.cpp asks th3Wearing()).
// Picked from the costume lab's REFLECT take, passes 11 to 15. An homage,
// so a name of its own. Earned by tapping the one red glyph that now and
// then falls in the DIGITAL rain -- see Theme::consumeRedGlyph().
//
// Included into squachy.cpp after the SHAMBLER's block, whose helpers
// (yz565, yzHash) it borrows. Drawn where drawBody() reaches, in order:
//   t3Fur()        body, legs and sleeves take the coat's colour
//   t3Back()       behind him: bullet-time rings, the cat
//   t3Coat()       over the torso and legs, under the arms
//   t3BehindHead() after the arms, before the head: the collar
//   t3Shades()     in place of his own shades
//   t3Front()      last: hands, the bullet, the spoon
//
// Everything that moves or talks is our own Squachy's only: a visitor in
// the coat is drawn wearing it, and does nothing else.

static const uint16_t T3_COAT    = yz565( 36,   0,  85);   // near-black, a violet cast
static const uint16_t T3_EDGE    = yz565(109,  36, 170);   // its edge, so it holds on black
static const uint16_t T3_G1      = yz565(182, 255, 170);   // the lens code's head
static const uint16_t T3_G2      = yz565(  0, 255,   0);
static const uint16_t T3_G3      = yz565(  0, 182,   0);
static const uint16_t T3_WHITE   = yz565(255, 255, 255);
static const uint16_t T3_RED     = yz565(255,   0,   0);
static const uint16_t T3_BLUE    = yz565(  0, 109, 255);
static const uint16_t T3_METAL   = yz565(182, 182, 170);
static const uint16_t T3_AIR     = yz565(109, 182, 255);   // bullet-time ripples

static uint16_t s_t3Fur = 0;
static uint32_t s_t3Now = 0;           // for the shades, which are not handed the clock
static bool     s_t3Own = false;       // this draw is our own Squachy, not a visitor

// The main screen's NEARBY headline is up for something serious.
static bool s_t3Nearby = false;
void setNearbyLit(bool on) { s_t3Nearby = on; }

// Bullet time: every fourteen seconds, for a second and a half. -1 outside
// it, else ms into it. The first one comes five seconds in.
static const uint32_t T3_BT_EVERY = 14000u, T3_BT_LEN = 1500u;
static inline int t3Bullet(uint32_t now) {
    const uint32_t p = (now + 9000u) % T3_BT_EVERY;
    return !Field::config.reduced && p < T3_BT_LEN ? (int)p : -1;
}

static const char* const T3_LINES[] = {
    "There is no spoon. There is a snack.",
    "I don't see the code anymore. Just AirTags.",
    "Free your mind. Keep your Bluetooth off.",
    "Deja vu. Same doorbell, same street.",
    "Whoa.",
    "I know kung fu. And where every Flock cam is.",
    "Follow the white rabbit. It's probably a Tile.",
    "Red pill, blue pill. I'll have both, thanks.",
};
static const uint8_t T3_LINES_N = sizeof(T3_LINES) / sizeof(T3_LINES[0]);
static const char* const T3_DODGE_LINES[] = { "Too slow.", "Missed me.", "Whoa.", "Not today." };
static const char* const T3_RED_LINES[]   = { "Red pill. Let's see how deep the burrow goes.",
                                              "Red. Down the rabbit hole. Bring snacks." };
static const char* const T3_BLUE_LINES[]  = { "Blue pill. Back to bed. Wake me if a Flock shows up.",
                                              "Blue. Believe whatever you want. I'll keep watching." };

// Where his lenses were drawn last, for the pill taps.
static int s_t3LensX[2] = { -9999, -9999 }, s_t3LensY = -9999, s_t3LensR = 0;
// Pills: which lens (-1 left/red, 1 right/blue, 0 none) and since when.
static int8_t   s_t3Pill = 0;
static uint32_t s_t3PillAt = 0;
static const uint32_t T3_PILL_MS = 2600u;
// When he ducked this bullet (0 = he hasn't).
static uint32_t s_t3DodgeAt = 0;
// The deja vu cat comes round this often; the emulator shortens it.
uint32_t t3DejaEvery = 240000u;

bool th3Wearing() { return currentOutfit() == OutfitId::TH3_0N3; }

// A tap on him, before it becomes a pet. True if the costume answered it.
static bool t3Tap(int x, int y, uint32_t now) {
    if (!th3Wearing()) return false;
    // In bullet time, any tap on him: he ducks under it.
    const int bt = t3Bullet(now);
    if (bt >= 0 && bt < (int)T3_BT_LEN - 300) {
        if (s_t3DodgeAt && now - s_t3DodgeAt < T3_BT_LEN) return true;
        s_t3DodgeAt = now ? now : 1;
        s_duckCooldown = 0;
        s_duckUntil = now + 1100u;
        mood = Mood::IDLE; moodUntil = 0;
        say(pick(T3_DODGE_LINES, 4), MIN_BUBBLE_MS);
        return true;
    }
    // On a lens: a pill shows in it.
    for (int k = 0; k < 2; k++) {
        const int dx = x - s_t3LensX[k], dy = y - s_t3LensY, rr = s_t3LensR + (int)(3 * s_lastScale);
        if (dx * dx + dy * dy <= rr * rr) {
            s_t3Pill = k == 0 ? -1 : 1;
            s_t3PillAt = now ? now : 1;
            say(k == 0 ? pick(T3_RED_LINES, 2) : pick(T3_BLUE_LINES, 2), 3200);
            return true;
        }
    }
    return false;
}

// Body, legs and sleeves the coat's colour; his hands keep his own fur.
static void t3Fur(uint16_t& main, uint16_t& light, uint16_t own, bool isOwn) {
    s_t3Fur = own;
    s_t3Own = isOwn;
    main = T3_COAT; light = T3_COAT;
}

static void t3Back(TFT_eSPI& t, int cx2, int hy, int ground, uint32_t now, Mood m, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    s_t3Now = now;
    if (!s_t3Own || Field::config.reduced) return;
    const int bt = t3Bullet(now);
    if (bt >= 0 && m != Mood::SLEEPY) {
        // Bullet time: rings of rippling air open out from behind him.
        for (int k = 0; k < 2; k++) {
            const int e = bt - k * 450;
            if (e < 0) continue;
            const int r = S(22) + e * S(40) / (int)T3_BT_LEN;
            t.drawEllipse(cx2, hy + S(24), r, r * 2 / 3, k == 0 ? T3_WHITE : T3_AIR);
        }
    }
    // A black cat walks past behind him. Then the same cat walks past again,
    // exactly the same, glitching as it goes. Deja vu. Half again his scale
    // and edged in violet, or it is lost in the rain.
    const float cs = scale * 1.5f;
    auto C = [cs](int v) { return (int)(v * cs); };
    const uint32_t p = (now + t3DejaEvery - 20000u % t3DejaEvery) % t3DejaEvery;
    const uint32_t walk = 2600u;
    static uint32_t s_dejaSaid = 0;
    if (p >= 3000u && p < 3400u && now - s_dejaSaid > 6000u) {
        s_dejaSaid = now;
        say("Deja vu.", 2200);
    }
    uint32_t e = 0xFFFFFFFFu;
    if (p < walk) e = p;
    else if (p >= 3000u && p < 3000u + walk) e = p - 3000u;
    if (e == 0xFFFFFFFFu) return;
    const int x = cx2 - C(40) + (int)(e * (uint32_t)C(80) / walk);
    const int y = ground - C(6);
    const bool step = (e / 180u) & 1u;
    for (int g = 1; g >= 0; g--) {
        const uint16_t c = g ? T3_EDGE : TFT_BLACK;
        t.fillEllipse(x, y, C(6) + g, C(3) + g, c);
        t.fillCircle(x + C(6), y - C(3), C(3) + g, c);
        t.fillTriangle(x + C(4) - g, y - C(5), x + C(5), y - C(8) - g, x + C(6) + g, y - C(5), c);
        t.fillTriangle(x + C(7) - g, y - C(5), x + C(8), y - C(8) - g, x + C(9) + g, y - C(5), c);
        for (int w = -g; w <= g; w++) t.drawLine(x - C(6), y - C(1) + w, x - C(10), y - C(6) + w, c);
        for (int k = 0; k < 2; k++)
            t.fillRect(x - C(4) + k * C(7) + (step ? C(1) : 0) - g, y + C(2), C(1) + 1 + 2 * g, C(4) + g, c);
    }
    t.fillRect(x + C(7), y - C(4), C(1), C(1), T3_G2);
    if (Settings::glitchEffects() && p >= 3000u && ((e / 120u) % 5u) == 0u)
        t.drawFastHLine(x - C(10), y - C(2), C(20), T3_G2);
}

// The coat: shoulders to ankles, flared, open down the front over a black
// shirt, the lapels folded back to show violet lining. Flat cutout pieces,
// like every other costume he wears.
static void t3Coat(TFT_eSPI& t, int cx2, int hy, uint32_t now, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    const int th = torsoHalf();
    const int top = hy + S(22);
    int hem = hy + S(53);
    const int lowFoot = (s_footLy > s_footRy) ? s_footLy : s_footRy;
    if (hem > lowFoot + S(2)) hem = lowFoot + S(2);
    // Flared, and the hem stirs: a breeze nobody else in the scene feels.
    const int hw = S(20) + (int)(sinf((float)(now % 1800u) / 1800.0f * 6.2831853f) * S(1));
    auto body = [&](int g, uint16_t c) {
        const int tw = S(th + 2) + g, bw = hw + g;
        t.fillTriangle(cx2 - tw, top - g, cx2 + tw, top - g, cx2 + bw, hem + g, c);
        t.fillTriangle(cx2 - tw, top - g, cx2 + bw, hem + g, cx2 - bw, hem + g, c);
    };
    body(1, TFT_BLACK);
    body(0, T3_COAT);
    // A violet edge down each side, so a near-black coat holds its shape on
    // the digital rain's black.
    t.drawLine(cx2 - S(th + 2), top + S(2), cx2 - hw, hem - S(1), T3_EDGE);
    t.drawLine(cx2 + S(th + 2), top + S(2), cx2 + hw, hem - S(1), T3_EDGE);
    // Open down the front: the shirt between the lapels.
    t.fillTriangle(cx2 - S(5), top, cx2 + S(5), top, cx2, hy + S(38), TFT_BLACK);
    // The lapels, folded back to show their lining. (As two thin lines they
    // read as a lanyard.)
    for (int8_t sg = -1; sg <= 1; sg += 2) {
        const int ax = cx2 + sg * S(5), ay = top;                // collar end
        const int bx = cx2 + sg * S(1), by = hy + S(37);         // the bottom of the V
        const int ox = cx2 + sg * S(10), oy = top + S(9);        // the fold's outer point
        t.fillTriangle(ax, ay - 1, ox + sg, oy, bx, by + 1, TFT_BLACK);
        t.fillTriangle(ax, ay, ox, oy, bx, by, Theme::VAPOR_PURPLE);
        t.drawLine(ax, ay, bx, by, Theme::blend(Theme::VAPOR_PURPLE, TFT_WHITE, 120));
    }
    // The split up the back from the hem shows the same lining.
    t.fillTriangle(cx2, hem - S(9), cx2 - S(3), hem, cx2 + S(3), hem, Theme::VAPOR_PURPLE);
    t.drawLine(cx2, hem - S(9), cx2 - S(3), hem, TFT_BLACK);
    t.drawLine(cx2, hem - S(9), cx2 + S(3), hem, TFT_BLACK);
    t.drawLine(cx2, hy + S(38), cx2, hem - S(9), TFT_BLACK);
    // Boots.
    const int fx[2] = { s_footLx, s_footRx }, fy[2] = { s_footLy, s_footRy };
    for (int k = 0; k < 2; k++) {
        t.fillRoundRect(fx[k] - 1, fy[k] - S(2) - 1, S(12) + 2, S(8) + 2, S(2), TFT_BLACK);
        t.fillRoundRect(fx[k], fy[k] - S(2), S(12), S(8), S(2), T3_COAT);
    }
}

// After the arms, before the head: the collar, turned up, poking out past
// his jaw either side.
static void t3BehindHead(TFT_eSPI& t, int cx2, int hh, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    for (int8_t sg = -1; sg <= 1; sg += 2) {
        t.fillTriangle(cx2 + sg * S(6), hh + S(28) + 1, cx2 + sg * (S(19) + 1), hh + S(28) + 1,
                       cx2 + sg * (S(21) + 1), hh + S(12) - 1, TFT_BLACK);
        t.fillTriangle(cx2 + sg * S(7), hh + S(27), cx2 + sg * S(19), hh + S(27),
                       cx2 + sg * S(21), hh + S(13), T3_COAT);
        t.drawLine(cx2 + sg * S(19), hh + S(27), cx2 + sg * S(21), hh + S(13), T3_EDGE);
    }
}

// In place of his own shades: the same chunky black frames and thick bridge,
// but round, with the code falling inside on dark green glass.
static void t3Shades(TFT_eSPI& t, int cx2, int hh, bool blink, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    const int ey = hh + S(10);
    const int R = S(5), r = S(4);
    const bool own = s_t3Own;
    if (own) { s_t3LensX[0] = cx2 - S(6); s_t3LensX[1] = cx2 + S(6); s_t3LensY = ey; s_t3LensR = R; }
    const int8_t pill = (own && s_t3Pill && s_t3Now - s_t3PillAt < T3_PILL_MS) ? s_t3Pill : 0;
    for (int8_t sg = -1; sg <= 1; sg += 2) t.fillCircle(cx2 + sg * S(6), ey, R, TFT_BLACK);
    t.fillRect(cx2 - S(2), ey - S(2), S(4), S(2), TFT_BLACK);
    // Every few seconds the lenses glitch the way the NEARBY headline does:
    // the code runs pink and the right lens tears sideways. In bullet time
    // the code stands still and the glitch holds off. While something
    // serious is nearby the code runs red, and faster: he has seen an Agent.
    const int bt = own ? t3Bullet(s_t3Now) : -1;
    const uint32_t lensT = Field::config.reduced ? 0 : bt >= 0 ? s_t3Now - (uint32_t)bt : s_t3Now;
    const bool glitch = Settings::glitchEffects() && !Field::config.reduced && (s_t3Now % 5200u) < 180u && bt < 0;
    const bool red = own && s_t3Nearby && !glitch;
    // Ducking a bullet he keeps his eyes on it: no blink.
    const bool ducking = own && s_t3DodgeAt && s_t3Now - s_t3DodgeAt < 1100u;
    for (int8_t sg = -1; sg <= 1; sg += 2) {
        const int lx = cx2 + sg * S(6) + ((glitch && sg > 0) ? S(1) : 0);
        if (blink && !ducking) continue;
        if (pill == sg) {
            // The pill you tapped for, glowing in this lens.
            t.fillCircle(lx, ey, r, sg < 0 ? yz565(73, 0, 0) : yz565(0, 0, 85));
            t.fillRoundRect(lx - r + S(1), ey - S(1), 2 * r - S(2), S(2) + 1, S(1), sg < 0 ? T3_RED : T3_BLUE);
            t.fillRect(lx - r + S(2), ey - S(1), S(1), 1, T3_WHITE);
            t.drawLine(lx - S(3), ey - S(3), lx - S(1), ey - S(4), T3_WHITE);
            continue;
        }
        t.fillCircle(lx, ey, r, glitch ? yz565(73, 0, 85) : red ? yz565(73, 0, 0) : yz565(0, 36, 0));
        for (int k = -1; k <= 1; k++) {
            const int x = lx + k * S(2);
            const int span = 2 * r;
            const int y = ey - r + (int)(((lensT / (red ? 45u : 70u)) + (uint32_t)(k + 1) * 5u + (sg > 0 ? 3u : 0u)) % (uint32_t)span);
            const int half = (int)sqrtf((float)(r * r - (x - lx) * (x - lx))) - 1;
            for (int d = 0; d < S(3); d++) {
                const int yy = y - d;
                if (yy > ey - half && yy < ey + half)
                    t.fillRect(x, yy, 2, 1, glitch ? (d == 0 ? T3_WHITE : Theme::VAPOR_PINK)
                                          : red    ? (d == 0 ? yz565(255, 182, 170) : d < S(2) ? T3_RED : yz565(182, 0, 0))
                                                   : (d == 0 ? T3_G1 : d < S(2) ? T3_G2 : T3_G3));
            }
        }
        // His shades' own glint, a white bar across the top left.
        t.drawLine(lx - S(3), ey - S(1), lx - S(1), ey - S(3), T3_WHITE);
    }
}

// Last of all: hands, the bullet, the spoon.
static void t3Front(TFT_eSPI& t, int cx2, int hh, uint32_t now, Mood m, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    // Hands in his own fur, at the end of the coat's sleeves.
    for (int8_t sg = -1; sg <= 1; sg += 2) {
        const int x1 = sg < 0 ? s_armL1x : s_armR1x, y1 = sg < 0 ? s_armL1y : s_armR1y;
        t.fillCircle(x1, y1, S(4) + 1, TFT_BLACK);
        t.fillCircle(x1, y1, S(4), s_t3Fur);
    }
    if (!s_t3Own) return;
    const int bt = t3Bullet(now);
    if (bt >= 0 && m != Mood::SLEEPY) {
        // One bullet creeps right to left past his head, trailing ripples.
        // Once he ducks, it keeps its line and his head drops out from under it.
        const int x = cx2 + S(34) - bt * S(68) / (int)T3_BT_LEN;
        static int s_btY = 0;
        const bool ducked = s_t3DodgeAt && now - s_t3DodgeAt < T3_BT_LEN;
        if (!ducked) s_btY = hh + S(4);
        const int y = s_btY;
        for (int r = 1; r <= 3; r++)
            t.drawEllipse(x + r * S(5), y, S(1) + r, S(2) + r * 2, r == 1 ? T3_WHITE : T3_AIR);
        t.fillRoundRect(x - S(4) - 1, y - S(2) - 1, S(8) + 2, S(4) + 2, S(2), TFT_BLACK);
        t.fillRoundRect(x - S(4), y - S(2), S(8), S(4), S(2), yz565(182, 146, 0));
        t.fillRoundRect(x - S(4), y - S(2), S(3), S(4), S(2), yz565(219, 182, 85));
    }
    if (s_grabbed) {
        // Picked up, he dangles from your finger bending a spoon, unbothered.
        // His hands are over his head, so it hangs down from one.
        const int x = s_armR1x, y = s_armR1y;
        const float bend = sinf((float)(now % 2400u) / 2400.0f * 6.2831853f);
        const int tipX = x + S(4) + (int)(bend * S(7)), tipY = y + S(16);
        wideLine(t, x, y + S(2), x + S(1), y + S(9), S(1) + 2, TFT_BLACK);
        wideLine(t, x + S(1), y + S(9), tipX, tipY, S(1) + 2, TFT_BLACK);
        t.fillEllipse(tipX, tipY + S(3), S(3) + 1, S(4) + 1, TFT_BLACK);
        wideLine(t, x, y + S(2), x + S(1), y + S(9), S(1), T3_METAL);
        wideLine(t, x + S(1), y + S(9), tipX, tipY, S(1), T3_METAL);
        t.fillEllipse(tipX, tipY + S(3), S(3), S(4), T3_METAL);
        // A glint that slides round the bowl as it bends.
        t.fillRect(tipX - S(1) + (int)(bend * S(1)), tipY + S(5), S(1), S(2), T3_WHITE);
        // At the full bend, a ripple off the bowl: the spoon isn't there.
        if (bend > 0.85f || bend < -0.85f) t.drawCircle(tipX, tipY + S(3), S(6), T3_EDGE);
    }
}
