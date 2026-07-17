# SEI-1618 PLL Controller (ESP32-C3-Zero)

PlatformIO firmware for an ESP32-C3-Zero board that programs a
**SEI-1618PG** fractional PLL synthesizer (driving an external
**MC12034A** dual-modulus prescaler) to lock a VCO to **1656 MHz** or
**1728 MHz** from a **10 MHz** reference.

The ESP32-C3-Zero only breaks out GPIO0-10, GPIO20, GPIO21, and
dedicates GPIO9 to its onboard BOOT button and GPIO10 to its onboard
WS2812 RGB LED — the pin map below avoids both, plus the usual
strapping pins (GPIO2/8), leaving GPIO20/21 free as headroom.

Datasheets for all chips referenced below are in [`pdf/`](pdf/):
`SEI1618.pdf`, `MC12034A.PDF`, plus supporting RF-path parts
`V585ME51-LF.pdf` (VCO), `hmc188.pdf`, `AD797.pdf` (likely loop-filter
op-amp) that were in the source folder.

## What this firmware does

* Bit-bangs the SEI1618's DATA/CLK/LE serial interface (see
  [`src/SEI1618.h`](src/SEI1618.h) / `SEI1618.cpp`).
* Loads the "constants" register word (MP, PP, R, F) once at boot.
* Loads the "channel" register word (K, M, N) for whichever of the two
  target frequencies is selected, via a button (`PIN_FREQ_SELECT`) or
  serial command (`1` / `2`).
* Reads the LOCK pin and mirrors it on an LED / prints state changes to
  serial.

Pin assignments are centralized in [`include/Pins.h`](include/Pins.h).

## Register derivation

From `pdf/SEI1618.pdf` (device control equations, p.6):

```
FR   = Fref / (R+1)              phase comparison frequency
Fout = [N*P + M + K/(F+1)] * FR  output frequency, P = external prescaler's lower ratio
```

Chosen operating point:

| Parameter | Value | Why |
|---|---|---|
| `Fref` | 10 MHz | given reference |
| `R` | 9 | `R+1=10` -> `FR = 1 MHz` |
| MC12034A `SW` | tied to VCC | selects the ÷32/33 pair -> `P = 32` |
| `F` | 0 | `f_frac = 1`, i.e. plain integer-N mode |

Both 1656 MHz and 1728 MHz divide evenly by `FR = 1 MHz` and `P = 32`
(their GCD is 72 MHz), so no fractional numerator is needed — `K = 0`
on both channels, which keeps the design simple and avoids fractional
spurs entirely:

| Channel | N | M | K | Check: (N·32+M)·1MHz |
|---|---|---|---|---|
| 1656 MHz | 51 | 24 | 0 | 1656 MHz |
| 1728 MHz | 54 | 0 | 0 | 1728 MHz |

`N` and `M` both satisfy the required `N > M` (needed for the
pulse-swallow prescaler sequencing) and fit their field widths (`N`:
8 bits / 0-255, `M`: 5 bits / 0-31).

`MP = 0` because the MC12034A's functional table shows the lower
divide ratio (32) is selected when its `MC` input is **high**, which
is the SEI1618's default polarity assumption for `MP = 0`.

`PP = 0` (positive `Kvco`) is an **assumption** — the correct polarity
depends on the VCO's tuning slope and which loop-filter op-amp input
the SEI1618's `φR`/`φV`/`Z` outputs feed. Verify against the VCO
(`V585ME51-LF`) and loop-filter (`AD797`?) design before first power-up;
if the loop doesn't converge or locks with the wrong sign of feedback,
flip `PP` to `1` in `main.cpp`.

If a different reference or channel set is needed later, recompute
`N`/`M`/`K` from the equations above — `R`/`F`/`P` only need to change
if the reference frequency or prescaler ratio changes.

## Wiring

### ESP32-C3-Zero <-> SEI-1618PG (digital control, 3.3 V shared logic)

Power the SEI1618 from **3.3 V** (it accepts 2.5-5.5 V) so its I/O
matches the ESP32-C3's logic levels directly — no level shifters
needed on the control lines.

| ESP32-C3-Zero pin | SEI1618 pin | Signal |
|---|---|---|
| GPIO4 | 18 | DATA |
| GPIO5 | 17 | CLK |
| GPIO6 | 16 | LE |
| GPIO7 | 19 | LOCK (input, active low = locked) |
| GPIO3 | 13 | PDWN (drive low = oscillator stage active) |

Optional:

| ESP32-C3-Zero pin | Function |
|---|---|
| GPIO1 | Channel-select button to GND (`INPUT_PULLUP`) |
| GPIO0 | Lock-status LED (through a series resistor) |

GPIO9 (onboard BOOT button) and GPIO10 (onboard WS2812 RGB LED) are
intentionally left unused by this firmware to avoid conflicting with
the board's built-in peripherals — see [`include/Pins.h`](include/Pins.h).

### SEI1618 <-> MC12034A (RF prescaler loop)

| SEI1618 pin | MC12034A pin | Notes |
|---|---|---|
| 10 MC | 6 MC | direct connect; MC12034A's `VIH1` min is 2.0 V, met by SEI1618's 3.3 V output regardless of the prescaler's own 5 V supply |
| 11 F_IN | 4 OUT | AC-couple, bias to +2 V ± 0.15 V per SEI1618 note 1 (`F_IN` min sensitivity 0.5 Vpp) |
| — | 3 SW | tie to VCC (not GPIO-driven) to select the ÷32/33 pair |
| — | 1/8 IN/n̄IN | from VCO output, AC-coupled per MC12034A datasheet Figure 4 |

MC12034A needs its own **4.5-5.5 V** supply (separate from the 3.3 V
SEI1618/ESP32-C3 rail) — its 2.0 GHz toggle frequency spec is only
guaranteed at that supply range.

### Other SEI1618 hardware notes

* `BIAS1` (pin 8) and `BIAS2` (pin 12) each need a 10 kΩ pull-up to
  `VDD`.
* `OSC_IN` (pin 20) takes the external 10 MHz reference (AC-coupled);
  `OSC_OUT` (pin 1) is left unconnected since an external reference
  source is used instead of a crystal on the on-chip amplifier.
* `φR`/`Z`/`φV`/`FV`/`FR` (pins 2-5, 7) feed the external loop filter
  / VCO — not covered by this firmware, see `V585ME51-LF.pdf` /
  `AD797.pdf` for that part of the design.

## Open question

`hmc188.pdf` (a Hittite wideband divider/prescaler MMIC) was included
alongside the other datasheets but its role in the signal chain isn't
specified. The register math above assumes the MC12034A sees the VCO's
fundamental output directly. **If an HMC188 ÷2 stage sits between the
VCO and the MC12034A, the effective `P` doubles and `N`/`M` must be
recalculated** (halve the target `Fout` in the equations above before
solving for `N`/`M`). Confirm the RF chain topology before finalizing
the PCB.

## Building

```
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial console (115200 baud)
```
