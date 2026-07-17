#pragma once

// ESP32-C3 mini <-> SEI-1618PG wiring.
// Avoids GPIO2/8/9 (strapping pins) and GPIO18-21 (USB-JTAG / UART0)
// so the board boots normally regardless of pin state at reset.
//
// See README.md for the full wiring table, including the fixed
// (non-GPIO) MC12034A and analog connections.

constexpr uint8_t PIN_SEI_DATA = 4;   // -> SEI1618 pin 18 DATA
constexpr uint8_t PIN_SEI_CLK  = 5;   // -> SEI1618 pin 17 CLK
constexpr uint8_t PIN_SEI_LE   = 6;   // -> SEI1618 pin 16 LE
constexpr uint8_t PIN_SEI_LOCK = 7;   // <- SEI1618 pin 19 LOCK (low = locked)
constexpr uint8_t PIN_SEI_PDWN = 10;  // -> SEI1618 pin 13 PDWN (active high; drive low)

constexpr uint8_t PIN_FREQ_SELECT = 1; // momentary button to GND, INPUT_PULLUP, toggles 1656/1728 MHz
constexpr uint8_t PIN_LOCK_LED    = 3; // optional external LED (with series resistor) mirrors lock state
