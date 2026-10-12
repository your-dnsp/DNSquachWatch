// ---- SHAMBLER: the zombie -----------------------------------------------------
// Grey-green from head to toe, a torn brown shirt over torn blue jeans, a
// stitched scar, a bandage coming loose, one tatty sneaker and one bare foot,
// a cracked lens, and three flies that will not leave him alone. Picked from
// twelve candidates in the zombie lab (2026-10-03).
//
// Unlocked by the owl on the FIRE background: tap him while he is asking
// WHAT REEKS?! -- see Theme::consumeOwlReek() and unlockShambler().
//
// Included into squachy.cpp after OVER 9000's block, whose helpers (yz565,
// yzAcross) it borrows. Drawn in four places, in the order drawBody() reaches
// them:
//   shFur()     his fur, head to toe, goes the colour of the grave
//   shBody()    jeans and shirt, over the torso and legs, under the arms
//   face        the face patch takes SH_SKIN, and the blush is left off
//   shFront()   last: scar, bandage, sleeve stump, shoes, lens, flies
//
// Colours are on the 8-bit panel's ramp, as everywhere else.
static const uint16_t SH_FUR      = yz565(109, 146,  85);
static const uint16_t SH_FURLT    = yz565(146, 182,  85);
static const uint16_t SH_SKIN     = yz565(182, 219, 170);
static const uint16_t SH_SHIRT    = yz565(146,  73,   0);
static const uint16_t SH_SHIRTDK  = yz565(109,  36,   0);
static const uint16_t SH_JEANS    = yz565( 36,  73, 170);
static const uint16_t SH_JEANSDK  = yz565(  0,  36,  85);
static const uint16_t SH_WHITE    = yz565(255, 255, 255);
static const uint16_t SH_GREY     = yz565(182, 182, 170);
static const uint16_t SH_RED      = yz565(219,   0,   0);
static const uint16_t SH_MUD      = yz565(109,  73,   0);

static int s_shHy = 0;

// Jeans torn off ragged above his knees, a gap between his legs.
static void shJeans(TFT_eSPI& t, int cx2, int hy, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    const int th = torsoHalf();
    const int top = hy + S(36), bot = hy + S(46);
    const int x0 = cx2 - S(th), w = S(2 * th);
    t.fillRect(x0 - 1, top, w + 2, bot - top + 1, TFT_BLACK);
    t.fillRect(x0, top, w, bot - top, SH_JEANS);
    t.fillTriangle(cx2 - S(2), bot, cx2 + S(2), bot, cx2, bot - S(4), SH_FUR);
    t.drawLine(cx2 - S(2), bot - 1, cx2, bot - S(4) - 1, TFT_BLACK);
    t.drawLine(cx2 + S(2), bot - 1, cx2, bot - S(4) - 1, TFT_BLACK);
    for (int i = 0; i < 4; i += 3) {            // the outer two quarters; the gap took the middle
        const int ax = x0 + i * w / 4, bx = x0 + (i + 1) * w / 4, mx = (ax + bx) / 2;
        t.fillTriangle(ax, bot, bx, bot, mx, bot + S(3), SH_JEANS);
        t.drawLine(ax, bot, mx, bot + S(3), TFT_BLACK);
        t.drawLine(bx, bot, mx, bot + S(3), TFT_BLACK);
    }
    t.fillRect(x0 + S(2), top + S(2), S(1), bot - top - S(4), SH_JEANSDK);
    // A torn knee, and mud.
    const int kx = cx2 + S(5), ky = hy + S(42);
    t.fillEllipse(kx, ky, S(3) + 1, S(2) + 1, TFT_BLACK);
    t.fillEllipse(kx, ky, S(3), S(2), SH_FUR);
    t.drawFastHLine(kx - S(3), ky - S(2), S(6), SH_WHITE);
    t.fillCircle(cx2 - S(6), hy + S(41), S(2), SH_MUD);
    t.fillCircle(cx2 - S(4), hy + S(43), S(1), SH_MUD);
}

// The shirt, its hem torn in four notches that show the jeans, a ragged hole
// low on his left side that shows the fur.
static void shShirt(TFT_eSPI& t, int cx2, int hy, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    const int th = torsoHalf();
    const int top = hy + S(21), hem = hy + S(39);
    const int x0 = cx2 - S(th + 2), w = S(2 * th + 4);
    t.fillRoundRect(x0 - 1, top - 1, w + 2, hem - top + 2, S(4), TFT_BLACK);
    t.fillRoundRect(x0, top, w, hem - top, S(4), SH_SHIRT);
    static const int8_t N[4][3] = { { -10, -6, 5 }, { -3, 1, 3 }, { 3, 8, 6 }, { 9, 13, 3 } };
    for (const auto& n : N) {
        const int ax = cx2 + S(n[0]), bx = cx2 + S(n[1]), mx = (ax + bx) / 2, my = hem - S(n[2]);
        t.fillTriangle(ax, hem, bx, hem, mx, my, SH_JEANS);
        t.drawLine(ax, hem - 1, mx, my - 1, TFT_BLACK);
        t.drawLine(bx, hem - 1, mx, my - 1, TFT_BLACK);
    }
    const int hx = cx2 - S(6), hy2 = top + S(10);
    static const int8_t R[6][2] = { { -3, -2 }, { 0, -4 }, { 3, -1 }, { 4, 2 }, { 0, 4 }, { -4, 2 } };
    auto out1 = [](int v) { return (v > 0) - (v < 0); };
    for (int i = 0; i < 6; i++) {
        const int j = (i + 1) % 6;
        t.fillTriangle(hx, hy2, hx + S(R[i][0]) + out1(R[i][0]), hy2 + S(R[i][1]) + out1(R[i][1]),
                       hx + S(R[j][0]) + out1(R[j][0]), hy2 + S(R[j][1]) + out1(R[j][1]), TFT_BLACK);
    }
    for (int i = 0; i < 6; i++) {
        const int j = (i + 1) % 6;
        t.fillTriangle(hx, hy2, hx + S(R[i][0]), hy2 + S(R[i][1]), hx + S(R[j][0]), hy2 + S(R[j][1]), SH_FUR);
    }
    t.fillRect(x0 + w - S(3), top + S(3), S(2), hem - top - S(8), SH_SHIRTDK);
}

static void shBody(TFT_eSPI& t, int cx2, int hy, float scale) {
    s_shHy = hy;
    shJeans(t, cx2, hy, scale);
    shShirt(t, cx2, hy, scale);
}

// The crack across his right lens, kept inside the lens (cx2+3..+11 across,
// hh+7..+12 down). drawBody() calls it right after the lenses, and only
// while they are on and open.
static void shCrack(TFT_eSPI& t, int cx2, int hh, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    t.drawLine(cx2 + S(4), hh + S(8), cx2 + S(7), hh + S(10), SH_WHITE);
    t.drawLine(cx2 + S(7), hh + S(10), cx2 + S(6), hh + S(11), SH_WHITE);
    t.drawLine(cx2 + S(7), hh + S(10), cx2 + S(10), hh + S(9), SH_WHITE);
}

// Last of all, from drawBody(): everything that sits on top of him.
static void shFront(TFT_eSPI& t, int cx2, int hh, uint32_t now, Mood m, float scale) {
    if (Field::config.reduced) now = 0; // Freeze flies and loose bandage motion.
    auto S = [scale](int v) { return (int)(v * scale); };

    // The scar across his forehead, stitched.
    {
        const int x0 = cx2 - S(10), y0 = hh + S(4), x1 = cx2 + S(6), y1 = hh + S(2);
        t.drawLine(x0, y0, x1, y1, TFT_BLACK);
        for (int i = 1; i < 5; i++) {
            const int x = x0 + (x1 - x0) * i / 5, y = y0 + (y1 - y0) * i / 5;
            t.drawFastVLine(x, y - S(1), 2 * S(1) + 1, TFT_BLACK);
        }
    }

    // A bandage round his left forearm, its loose end swinging.
    {
        const int x0 = s_armL0x, y0 = s_armL0y, x1 = s_armL1x, y1 = s_armL1y;
        yzAcross(t, x0, y0, x1, y1, 48, 76, S(4) + 1, S(4) + 1, TFT_BLACK);
        yzAcross(t, x0, y0, x1, y1, 50, 74, S(4), S(4), SH_WHITE);
        yzAcross(t, x0, y0, x1, y1, 58, 60, S(4), S(4), SH_GREY);
        yzAcross(t, x0, y0, x1, y1, 66, 68, S(4), S(4), SH_GREY);
        const int ex = x0 + (x1 - x0) * 74 / 100, ey = y0 + (y1 - y0) * 74 / 100;
        const int sw = (int)(sinf((float)(now % 1400u) / 1400.0f * 6.2831853f) * S(2));
        wideLine(t, ex, ey, ex - S(4) + sw, ey + S(7), S(1) + 1, SH_WHITE);
    }

    // What is left of his right sleeve: a short cuff at the shoulder with two
    // tatters hanging off it.
    {
        const int x0 = s_armR0x, y0 = s_armR0y, x1 = s_armR1x, y1 = s_armR1y;
        yzAcross(t, x0, y0, x1, y1, 4, 22, S(4) + 1, S(4) + 1, TFT_BLACK);
        yzAcross(t, x0, y0, x1, y1, 5, 21, S(4), S(4), SH_SHIRT);
        const float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
        const float len = sqrtf(dx * dx + dy * dy) + 0.01f;
        for (int8_t sd = -1; sd <= 1; sd += 2) {
            const int bx = x0 + (int)(dx * 0.21f + (-dy / len) * sd * S(2));
            const int by = y0 + (int)(dy * 0.21f + (dx / len) * sd * S(2));
            const int tx = bx + (int)(dx / len * S(5)), ty = by + (int)(dy / len * S(5));
            t.fillTriangle(bx - S(1) - 1, by, bx + S(1) + 1, by, tx, ty + 1, TFT_BLACK);
            t.fillTriangle(bx - S(1), by, bx + S(1), by, tx, ty, SH_SHIRT);
        }
    }

    // One tatty sneaker on his left foot; the right one is long gone.
    {
        const int lx = s_footLx, ly = s_footLy;
        t.fillRoundRect(lx - 1, ly - S(3) - 1, S(12) + 2, S(9) + 2, S(3), TFT_BLACK);
        t.fillRoundRect(lx, ly - S(3), S(12), S(9), S(3), SH_WHITE);
        t.fillRect(lx, ly + S(4), S(12), S(2), SH_RED);
        t.drawLine(lx + S(3), ly - S(1), lx + S(7), ly + S(1), TFT_BLACK);
        t.drawLine(lx + S(3), ly + S(1), lx + S(7), ly - S(1), TFT_BLACK);
        const int rx = s_footRx, ry = s_footRy;
        for (int k = 0; k < 4; k++) {
            const int tx = rx + S(2) + k * S(3), ty = ry + S(4);
            t.fillCircle(tx, ty, S(1) + 1, TFT_BLACK);
            t.fillCircle(tx, ty, S(1), SH_FURLT);
        }
    }

    if (m == Mood::SLEEPY) return;              // the flies sleep when he does
    // Three flies going round his head, wings flapping.
    for (int i = 0; i < 3; i++) {
        const float a = (float)(now % 1800u) / 1800.0f * 6.2831853f * (i == 1 ? -1.0f : 1.0f) + i * 2.1f;
        const int x = cx2 + (int)(cosf(a) * S(22)), y = hh - S(4) + (int)(sinf(a) * S(6)) + (int)(sinf(a * 3.0f) * S(2));
        const uint16_t wc = ((now / 50u + i) & 1) ? SH_WHITE : SH_GREY;
        t.fillRect(x - 3, y - 3, 3, 3, wc);
        t.fillRect(x + 2, y - 3, 3, 3, wc);
        t.fillRect(x - 1, y - 1, 4, 4, TFT_BLACK);
    }
    // A buzz mark beside them.
    if ((now / 400u) & 1) {
        const int zx = cx2 + S(18), zy = hh - S(8);
        t.drawLine(zx, zy, zx + S(3), zy, TFT_BLACK);
        t.drawLine(zx + S(3), zy, zx, zy + S(3), TFT_BLACK);
        t.drawLine(zx, zy + S(3), zx + S(3), zy + S(3), TFT_BLACK);
    }
    // Now and then one lands on his head for a rest.
    if ((now % 5000u) < 1400u) {
        const int fx = cx2 - S(2), fy = hh - S(1);
        t.fillRect(fx - 1, fy - 1, 4, 4, TFT_BLACK);
        t.fillRect(fx - 3, fy - 3, 3, 2, SH_GREY);
        t.fillRect(fx + 2, fy - 3, 3, 2, SH_GREY);
    }
}
