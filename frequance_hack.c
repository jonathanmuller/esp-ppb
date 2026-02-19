// timesync.c
//
// Purpose
//   Expose a single private entry point from the ESP32-C3 Wi‑Fi PHY library:
//     int get_rx_freq_local(void);
//   and explain what it returns and when to call it.
//
// How to call (short answer)
//   - Call get_rx_freq_local() immediately after a packet is received while the
//     RX results are still latched. The simplest place is inside your Wi‑Fi
//     promiscuous receive callback. Example snippet (comment‑only):
//       esp_wifi_set_promiscuous(true);
//       esp_wifi_set_promiscuous_rx_cb(my_cb);
//       void my_cb(void* buf, wifi_promiscuous_pkt_type_t t) {
//         int step = get_rx_freq_local ? get_rx_freq_local() : -1;
//         /* use 'step' */
//       }
//   - Do not call it long after RX from an arbitrary task; per‑packet registers
//     may have been overwritten by the next frame.
//
// What the return value means (simple words)
//   - Radios aren’t perfectly tuned; your device’s frequency can be a little
//     higher or lower than the sender’s. The receiver measures “how far off”
//     the frequency is for the current packet. get_rx_freq_local() returns a
//     number that represents how many correction steps the hardware thinks are
//     needed to line up with the sender. Bigger number → bigger mismatch; zero
//     or small number → you’re already close. It’s NOT in Hz directly; it’s a
//     bucketed step value used by the internal PLL correction code.
//
// What the return value looks like (from the dump)
//   - The code tail‑calls a small table/logic that maps the raw measurement to
//     discrete steps such as 39, 36, 33, …, 0. Think of these as “coarse
//     correction units.” The actual size of a step in Hz depends on channel
//     bandwidth and modem settings, so the library keeps it abstract.
//
// How the CFO is estimated (deeper but still friendly)
//   1) Every Wi‑Fi packet starts with a training preamble (short and long
//      training fields, STF/LTF) made of known tones. If your radio is slightly
//      fast or slow, the phase of those tones rotates steadily over time.
//   2) The PHY measures that rotation to estimate frequency error (CFO).
//   3) In this build, hardware/firmware stores two pieces in RX control
//      registers per packet: a coarse 2‑bit part and a fine 7‑bit part. If the
//      coarse path isn’t flagged, a 5‑bit reduced value is used instead.
//   4) get_rx_freq_local() reads those latched bits, builds a raw “code” and
//      immediately maps it via the internal converter, then returns the mapped
//      step to you. You just get the ready‑to‑use discrete step number.
//
// Practical interpretation
//   - Use the return value to gauge link clock mismatch or to feed your own
//     time/frequency synchronization logic. Treat larger values as “further
//     away”; treat near‑zero as “already well aligned.” If you need absolute Hz
//     or ppm, you must empirically calibrate how one step translates to Hz on
//     your channel/rate, because the library doesn’t expose that scale.
//
// Provenance (from /home/jonathan/Desktop/timesync/assembly.txt)
//   - get_rx_freq_local symbol header at: "Disassembly of section .text.get_rx_freq_local"
//   - It checks bits of RXCTRL[0]/[1], forms a code, then tail‑jumps through a
//     function pointer (offset +220 in a dispatch table) to the converter named
//     freq_offset_table (see: "Disassembly of section .text.freq_offset_table").

#include <stdint.h>

// Single private symbol we rely on.
// Marked weak so this compiles even if the symbol is stripped; be sure your
// firmware actually provides it at link/runtime for real use.
__attribute__((weak)) int get_rx_freq_local(void);

// No wrappers on purpose: call get_rx_freq_local() directly from your code.
