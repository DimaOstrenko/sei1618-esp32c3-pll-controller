#pragma once
#include <Arduino.h>

// Driver for the SEI-1618PG fractional PLL synthesizer.
//
// Two serial words program the device (MSB first, latched on the CLK
// rising edge, transferred to the internal latches on the LE low->high
// transition):
//
//   constants word (14 bits): MP PP R[4:0] F[5:0] CB=1
//   frequency word (20 bits): K[5:0] M[4:0] N[7:0] CB=0
//
// The constants word only needs to be sent once after power-up (or
// whenever MP/PP/R/F change); the frequency word is resent for every
// channel change. See pdf/SEI1618.pdf pages 5-8 for the register
// definitions and the governing equation:
//
//   Fout = [N*P + M + K/(F+1)] * (Fref/(R+1))
//
// where P is the external MC12034A prescaler's lower divide ratio.

struct SEI1618Constants {
    uint8_t MP; // modulus control polarity: 0 = lower divide ratio when MC is high
    uint8_t PP; // phase detector polarity: 0 = positive Kvco
    uint8_t R;  // reference divider vector, 0-31 (reference division = R+1)
    uint8_t F;  // fractionality denominator vector, 0-63 (fractionality = F+1)
};

struct SEI1618Channel {
    uint8_t K; // fractional numerator, 0-63
    uint8_t M; // dual-modulus modulating counter, 0-31 (must be < N)
    uint8_t N; // VCO dividing counter, 0-255
};

class SEI1618 {
public:
    SEI1618(uint8_t dataPin, uint8_t clkPin, uint8_t lePin, uint8_t lockPin);

    void begin();
    void writeConstants(const SEI1618Constants &c);
    void writeChannel(const SEI1618Channel &ch);
    bool isLocked() const; // LOCK pin is active low (low = locked)

private:
    uint8_t _data, _clk, _le, _lock;

    void shiftOutMSBFirst(uint32_t value, uint8_t numBits);
    void latch();
};
