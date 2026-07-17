#include "SEI1618.h"

// Bit-bang delay: the SEI1618 accepts up to 5 MHz CLK at 3V (min pulse
// width 50 ns, setup/hold ~10 ns). A few microseconds per half-cycle is
// far slower than required but keeps this robust against Arduino GPIO
// jitter without needing precise timing.
static constexpr uint32_t BIT_DELAY_US = 2;

SEI1618::SEI1618(uint8_t dataPin, uint8_t clkPin, uint8_t lePin, uint8_t lockPin)
    : _data(dataPin), _clk(clkPin), _le(lePin), _lock(lockPin) {}

void SEI1618::begin() {
    pinMode(_data, OUTPUT);
    pinMode(_clk, OUTPUT);
    pinMode(_le, OUTPUT);
    pinMode(_lock, INPUT);

    digitalWrite(_data, LOW);
    digitalWrite(_clk, LOW);
    digitalWrite(_le, LOW);
}

void SEI1618::shiftOutMSBFirst(uint32_t value, uint8_t numBits) {
    for (int8_t i = numBits - 1; i >= 0; i--) {
        digitalWrite(_data, (value >> i) & 0x1);
        delayMicroseconds(BIT_DELAY_US);
        digitalWrite(_clk, HIGH); // data is latched into the shift register on this edge
        delayMicroseconds(BIT_DELAY_US);
        digitalWrite(_clk, LOW);
        delayMicroseconds(BIT_DELAY_US);
    }
}

void SEI1618::latch() {
    digitalWrite(_le, HIGH); // low->high transfers the shift register into the internal latches
    delayMicroseconds(BIT_DELAY_US);
    digitalWrite(_le, LOW);
}

void SEI1618::writeConstants(const SEI1618Constants &c) {
    uint32_t word = (uint32_t(c.MP & 0x1) << 13) |
                    (uint32_t(c.PP & 0x1) << 12) |
                    (uint32_t(c.R & 0x1F) << 7) |
                    (uint32_t(c.F & 0x3F) << 1) |
                    0x1; // CB = 1 selects the constants latch (MP,PP,R,F)
    shiftOutMSBFirst(word, 14);
    latch();
}

void SEI1618::writeChannel(const SEI1618Channel &ch) {
    uint32_t word = (uint32_t(ch.K & 0x3F) << 14) |
                     (uint32_t(ch.M & 0x1F) << 9) |
                     (uint32_t(ch.N & 0xFF) << 1) |
                     0x0; // CB = 0 selects the frequency latch (K,M,N)
    shiftOutMSBFirst(word, 20);
    latch();
}

bool SEI1618::isLocked() const {
    return digitalRead(_lock) == LOW;
}
