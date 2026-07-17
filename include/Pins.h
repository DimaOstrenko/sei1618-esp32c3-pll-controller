#pragma once

// ESP32-C3-Zero <-> SEI-1618PG wiring.
//
// The Zero board only breaks out GPIO0-10, GPIO20, GPIO21. Within that
// set this avoids:
//   - GPIO2/8  strapping pins (must not be pulled low at boot)
//   - GPIO9    onboard BOOT button (strapping pin; already committed)
//   - GPIO10   onboard WS2812 RGB LED (single-wire NeoPixel protocol,
//              not a plain digitalWrite target)
//   - GPIO20/21 left free as headroom / possible debug UART
//
// See README.md for the full wiring table, including the fixed
// (non-GPIO) MC12034A and analog connections.

constexpr uint8_t PIN_SEI_DATA = 4;   // -> SEI1618 pin 18 DATA
constexpr uint8_t PIN_SEI_CLK  = 5;   // -> SEI1618 pin 17 CLK
constexpr uint8_t PIN_SEI_LE   = 6;   // -> SEI1618 pin 16 LE
constexpr uint8_t PIN_SEI_LOCK = 7;   // <- SEI1618 pin 19 LOCK (low = locked)
constexpr uint8_t PIN_SEI_PDWN = 3;   // -> SEI1618 pin 13 PDWN (active high; drive low)

constexpr uint8_t PIN_FREQ_SELECT = 1; // momentary button to GND, INPUT_PULLUP, toggles 1656/1728 MHz
constexpr uint8_t PIN_LOCK_LED    = 0; // optional external LED (with series resistor) mirrors lock state
