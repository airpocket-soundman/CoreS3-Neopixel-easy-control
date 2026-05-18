#include <M5Unified.h>
#include <Adafruit_NeoPixel.h>

// ===== Configuration =====
constexpr int LED_COUNT = 60;
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int FRAME_W = 14;          // outer indicator frame width

// ===== State =====
static uint8_t rgb_val[3] = {255, 128, 0};
static uint8_t bright    = 200;
static bool    ledOn     = false;
static bool    needsRedraw = true;

// ===== Hardware =====
static Adafruit_NeoPixel strip(LED_COUNT, 0, NEO_GRB + NEO_KHZ800);
static M5Canvas canvas(&M5.Display);

// ===== UI Layout =====
struct Slider {
    int       x, y, w, h;
    uint16_t  color;
    uint8_t*  value;
    const char* label;
};

constexpr int SL_X   = FRAME_W + 28;
constexpr int SL_W   = SCREEN_W - SL_X - 56 - FRAME_W;
constexpr int SL_H   = 24;
constexpr int SL_Y0  = 52;
constexpr int SL_GAP = 30;

static Slider sliders[] = {
    {SL_X, SL_Y0 + 0 * SL_GAP, SL_W, SL_H, 0xF800, &rgb_val[0], "R"},
    {SL_X, SL_Y0 + 1 * SL_GAP, SL_W, SL_H, 0x07E0, &rgb_val[1], "G"},
    {SL_X, SL_Y0 + 2 * SL_GAP, SL_W, SL_H, 0x001F, &rgb_val[2], "B"},
    {SL_X, SL_Y0 + 3 * SL_GAP, SL_W, SL_H, 0xFFFF, &bright,     "L"},
};
constexpr int N_SLIDERS = sizeof(sliders) / sizeof(sliders[0]);

constexpr int BTN_W = 96;
constexpr int BTN_H = 32;
constexpr int BTN_X = SCREEN_W - FRAME_W - BTN_W - 6;
constexpr int BTN_Y = SCREEN_H - FRAME_W - BTN_H - 6;

static int activeSliderIdx = -1;

// ===== Helpers =====
static inline bool inside(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// Apply brightness to a channel for the on-screen indicator (gives perceived intensity).
static inline uint8_t scaleCh(uint8_t v, uint8_t b) {
    return (uint16_t)v * b / 255;
}

// ===== Drawing =====
static void drawSlider(const Slider& s) {
    canvas.fillRoundRect(s.x, s.y, s.w, s.h, 6, 0x2104);                 // dark track
    int fillW = (s.w * (*s.value)) / 255;
    if (fillW > 0) {
        canvas.fillRoundRect(s.x, s.y, fillW, s.h, 6, s.color);
    }
    canvas.drawRoundRect(s.x, s.y, s.w, s.h, 6, TFT_WHITE);

    // knob
    int knobX = s.x + fillW;
    knobX = (knobX < s.x + 4) ? s.x + 4 : ((knobX > s.x + s.w - 4) ? s.x + s.w - 4 : knobX);
    canvas.fillCircle(knobX, s.y + s.h / 2, s.h / 2 + 2, TFT_WHITE);
    canvas.drawCircle(knobX, s.y + s.h / 2, s.h / 2 + 2, TFT_BLACK);

    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    canvas.setTextDatum(ML_DATUM);
    canvas.drawString(s.label, FRAME_W + 6, s.y + s.h / 2);

    canvas.setTextDatum(MR_DATUM);
    char buf[8];
    snprintf(buf, sizeof(buf), "%3d", *s.value);
    canvas.drawString(buf, SCREEN_W - FRAME_W - 6, s.y + s.h / 2);
}

static void drawUI() {
    canvas.fillSprite(TFT_BLACK);

    // 額縁インジケータ: RGB値×輝度を反映
    uint16_t frameColor = canvas.color565(
        scaleCh(rgb_val[0], bright),
        scaleCh(rgb_val[1], bright),
        scaleCh(rgb_val[2], bright));
    canvas.fillRect(0, 0, SCREEN_W, FRAME_W, frameColor);
    canvas.fillRect(0, SCREEN_H - FRAME_W, SCREEN_W, FRAME_W, frameColor);
    canvas.fillRect(0, 0, FRAME_W, SCREEN_H, frameColor);
    canvas.fillRect(SCREEN_W - FRAME_W, 0, FRAME_W, SCREEN_H, frameColor);

    // Title
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("NeoPixel", FRAME_W + 6, FRAME_W + 6);
    canvas.setTextSize(1);
    canvas.drawString(ledOn ? "OUTPUT: ON" : "OUTPUT: OFF",
                      FRAME_W + 6, FRAME_W + 26);

    // Sliders
    for (int i = 0; i < N_SLIDERS; ++i) drawSlider(sliders[i]);

    // ON/OFF button
    uint16_t btnBg = ledOn ? 0x07E0 : 0x39C7;
    canvas.fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, btnBg);
    canvas.drawRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, TFT_WHITE);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    canvas.setTextDatum(MC_DATUM);
    canvas.drawString(ledOn ? "ON" : "OFF",
                      BTN_X + BTN_W / 2, BTN_Y + BTN_H / 2);

    canvas.pushSprite(0, 0);
}

// ===== Input =====
static void updateSliderFromTouch(Slider& s, int tx) {
    int px = tx - s.x;
    if (px < 0) px = 0;
    if (px > s.w) px = s.w;
    uint8_t newVal = (px * 255 + s.w / 2) / s.w;
    if (*s.value != newVal) {
        *s.value = newVal;
        needsRedraw = true;
    }
}

static void handleTouch() {
    auto t = M5.Touch.getDetail();

    if (t.wasPressed()) {
        // ON/OFF button
        if (inside(t.x, t.y, BTN_X, BTN_Y, BTN_W, BTN_H)) {
            ledOn = !ledOn;
            needsRedraw = true;
            return;
        }
        // Slider hit-test (with expanded hitbox for easier touch)
        for (int i = 0; i < N_SLIDERS; ++i) {
            auto& s = sliders[i];
            if (inside(t.x, t.y, s.x - 8, s.y - 10, s.w + 16, s.h + 20)) {
                activeSliderIdx = i;
                updateSliderFromTouch(s, t.x);
                return;
            }
        }
    } else if (t.isPressed() && activeSliderIdx >= 0) {
        updateSliderFromTouch(sliders[activeSliderIdx], t.x);
    } else if (t.wasReleased()) {
        activeSliderIdx = -1;
    }
}

// ===== LED output =====
static void updateLEDs() {
    static uint8_t lr = 0xFF, lg = 0xFF, lb = 0xFF, ll = 0xFF;
    static bool    lon = !ledOn;

    if (rgb_val[0] == lr && rgb_val[1] == lg && rgb_val[2] == lb
        && bright == ll && ledOn == lon) return;

    lr = rgb_val[0]; lg = rgb_val[1]; lb = rgb_val[2];
    ll = bright;     lon = ledOn;

    if (ledOn) {
        strip.setBrightness(bright);
        uint32_t c = strip.Color(rgb_val[0], rgb_val[1], rgb_val[2]);
        for (int i = 0; i < LED_COUNT; ++i) strip.setPixelColor(i, c);
    } else {
        for (int i = 0; i < LED_COUNT; ++i) strip.setPixelColor(i, 0);
    }
    strip.show();
}

// ===== Setup / Loop =====
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Power.setExtOutput(true);  // Port A 5V supply ON

    canvas.setPsram(true);
    canvas.setColorDepth(16);
    canvas.createSprite(SCREEN_W, SCREEN_H);

    int ledPin = M5.getPin(m5::pin_name_t::port_a_scl);
    if (ledPin < 0) ledPin = 2;  // fallback
    strip.setPin(ledPin);
    strip.begin();
    strip.setBrightness(bright);
    strip.clear();
    strip.show();

    drawUI();
}

void loop() {
    M5.update();
    handleTouch();
    if (needsRedraw) {
        drawUI();
        needsRedraw = false;
    }
    updateLEDs();
    delay(5);
}
