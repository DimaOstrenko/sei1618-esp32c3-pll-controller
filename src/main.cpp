#include <Arduino.h>
#include "Pins.h"
#include "SEI1618.h"

// Frequency plan (see README.md "Register derivation" for the math):
//   Fref = 10 MHz, R = 9  -> phase comparison frequency FR = 1 MHz
//   MC12034A SW tied to VCC -> P = 32 (32/33 dual modulus)
//   F = 0 -> integer mode (K always 0); both targets divide FR evenly.
static const SEI1618Constants kConstants = {
    /* MP */ 0, // MC12034: lower ratio (32) selected when MC is high
    /* PP */ 0, // positive Kvco assumed -- verify against the VCO/loop-filter polarity
    /* R  */ 9,
    /* F  */ 0,
};

struct Channel {
    const char *name;
    double freqMHz;
    SEI1618Channel regs;
};

static const Channel kChannels[] = {
    {"1656 MHz", 1656.0, {/*K*/ 0, /*M*/ 24, /*N*/ 51}},
    {"1728 MHz", 1728.0, {/*K*/ 0, /*M*/ 0, /*N*/ 54}},
};
static constexpr size_t kNumChannels = sizeof(kChannels) / sizeof(kChannels[0]);

static SEI1618 pll(PIN_SEI_DATA, PIN_SEI_CLK, PIN_SEI_LE, PIN_SEI_LOCK);
static size_t activeChannel = 0;
static bool lastButtonState = HIGH;

static void selectChannel(size_t index) {
    activeChannel = index % kNumChannels;
    const Channel &ch = kChannels[activeChannel];
    pll.writeChannel(ch.regs);
    Serial.printf("Selected channel: %s (N=%u M=%u K=%u)\n",
                   ch.name, ch.regs.N, ch.regs.M, ch.regs.K);
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_SEI_PDWN, OUTPUT);
    digitalWrite(PIN_SEI_PDWN, LOW); // keep the reference input stage active

    pinMode(PIN_FREQ_SELECT, INPUT_PULLUP);
    pinMode(PIN_LOCK_LED, OUTPUT);
    digitalWrite(PIN_LOCK_LED, LOW);

    pll.begin();
    delay(10);
    pll.writeConstants(kConstants);
    selectChannel(0);

    Serial.println("SEI1618 PLL controller ready.");
    Serial.println("Press the select button, or send '1'/'2' over serial, to switch channels.");
}

void loop() {
    // Button: active-low, toggles channel on press.
    bool buttonState = digitalRead(PIN_FREQ_SELECT);
    if (lastButtonState == HIGH && buttonState == LOW) {
        selectChannel(activeChannel + 1);
    }
    lastButtonState = buttonState;

    // Serial: '1' or '2' selects a channel directly.
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '1') selectChannel(0);
        else if (c == '2') selectChannel(1);
    }

    bool locked = pll.isLocked();
    digitalWrite(PIN_LOCK_LED, locked ? HIGH : LOW);

    static bool lastLocked = false;
    static unsigned long lastReport = 0;
    if (locked != lastLocked || millis() - lastReport > 2000) {
        Serial.printf("%s: %s\n", kChannels[activeChannel].name, locked ? "LOCKED" : "unlocked");
        lastLocked = locked;
        lastReport = millis();
    }

    delay(20);
}
