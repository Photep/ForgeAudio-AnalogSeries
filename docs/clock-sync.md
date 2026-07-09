# Clock & Sync

Patch a clock into the CLK input and the LFO locks to your tempo. Unplug it and the LFO free-runs from the Rate knob.

## How sync behaves

| Situation | What happens |
|-----------|--------------|
| No clock patched | The LFO free-runs. The Rate knob sets the speed (0.01–20 Hz) and the display shows the rate in Hz. |
| Clock just connected | The LFO starts following the incoming pulses. The waveform restarts and the SYNC badge blinks while it learns the tempo. |
| Locked to tempo | The display shows the current ratio and the tempo in BPM, and the SYNC badge flashes on each clock pulse. |

Locking is fast. If the incoming tempo is close to one it recently tracked, it re-locks on the second pulse; otherwise it settles after a few steady pulses. Unplugging the clock returns to free-run instantly, and it remembers the last tempo for a quick re-lock.

Occasional glitchy pulses — ones that arrive far too fast or too slow — are ignored, so a stray edge won't throw off the tempo. But a real tempo change (several pulses in a row at the new speed) is followed: the LFO re-learns the new tempo instead of staying stuck on the old one.

The CLK and RESET inputs work with any standard clock or gate. They register on a rising signal above 1.0V and re-arm below 0.1V, so both 5V and 10V sources work.

## Clocked ratios

In clocked mode the Rate knob snaps to one of 15 musical ratios of the incoming tempo, from sixteen times slower to sixteen times faster. The center of the knob is **x1** — one LFO cycle per clock pulse.

| Ratio | Speed vs. clock |
|-------|-----------------|
| /16 | 16× slower |
| /8 | 8× slower |
| /6 | 6× slower |
| /4 | 4× slower |
| /3 | 3× slower |
| /2 | half speed |
| /1.5 | two-thirds speed |
| x1 | same as clock |
| x1.5 | 1.5× faster |
| x2 | 2× faster |
| x3 | 3× faster |
| x4 | 4× faster |
| x6 | 6× faster |
| x8 | 8× faster |
| x16 | 16× faster |

Resets stay aligned to the clock and are click-free. The RESET input restarts the waveform at the beginning of its cycle.
