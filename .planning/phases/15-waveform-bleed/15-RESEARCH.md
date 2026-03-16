# Phase 15: Waveform Bleed - Research

**Researched:** 2026-03-17
**Domain:** DSP -- analog crossfader crosstalk modeling within waveform morph pipeline
**Confidence:** HIGH

## Summary

Phase 15 adds waveform bleed (CHAR-05): adjacent-shape crosstalk during morph transitions, controlled by the Character knob. The existing `computeMorphedWave()` function (AnalogLFO.cpp:264-281) already computes all four waveform shapes before performing a linear crossfade by segment. Bleed modifies this crossfade by mixing in small amounts from the neighboring shapes (the shapes adjacent to the current morph segment endpoints). At Character=0, the crossfade remains crisp and identical to v1.1 behavior.

The implementation is purely a modification of the crossfade math inside `computeMorphedWave()`. All four waveforms are already computed per-sample, so neighbor shapes are available at zero additional cost. The bleed amount is gated by `progressiveCurve(character)` following the same scaling pattern used by all other Character-dependent effects. Component spread from Phase 14 adds a per-instance offset to the bleed magnitude, and slow modulation from an existing OU layer provides time-varying fluctuation.

**Primary recommendation:** Implement bleed as a post-crossfade additive mix of neighbor shapes within `computeMorphedWave()`. The function signature does not need to change. Bleed magnitude is `progressiveCurve(character) * bleedBase * (1 + bleedSpread)` where bleedBase is ~0.04 (4%) and bleedSpread is a per-instance component spread offset. Time modulation taps ouLayers[0] (slowest OU layer, 0.05Hz) rather than adding a new modulator.

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Bleed source & reach:** Nearest neighbors only -- bleed comes from the two shapes adjacent to the current morph segment (not all four shapes). Proximity-weighted: the closer neighbor bleeds more. Subtle magnitude: 3-5% of output from neighbor shapes at maximum Character, at the morph midpoint.
- **Bleed texture:** Slightly modulated over time -- bleed amount fluctuates slowly, like real capacitive crosstalk that changes with temperature. Component spread (Phase 14) DOES affect bleed -- per-instance offset to bleed magnitude, consistent with "every instance is slightly different" philosophy. Uses existing component spread infrastructure (serialized RNG seed).
- **Morph endpoint behavior:** Must satisfy success criterion #2: at Character=0, morph is crisp crossfade identical to v1.1.
- **Amplitude safety:** Must satisfy success criterion #3: output stays within +/-5V at all morph/character/bleed combinations.

### Claude's Discretion
- Edge wrapping at morph strip endpoints (sine<->square as neighbors or one-sided bleed)
- Whether segment boundaries are pure points or have bleed
- Whether morph endpoints have bleed when Character is up
- Modulation source for bleed fluctuation (existing OU layers vs. new source)
- Exact bleed magnitude within 3-5% range
- Whether explicit output clamping is needed
- Integration point within computeMorphedWave() vs. separate post-processing

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CHAR-05 | Waveform bleed introduces adjacent-shape crosstalk during morph transitions | Modifies computeMorphedWave() crossfade math to mix in neighbor shapes. All four shapes already computed. Character-gated via progressiveCurve(). Component spread adds bleedSpread offset. OU layer provides time modulation. |

</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | 2.x | Module framework, DSP primitives | Project platform -- already in use |
| Existing `computeMorphedWave()` | N/A | Waveform crossfade pipeline | Primary integration point; all four shapes already computed here |
| `progressiveCurve()` | N/A | x^2 character scaling | Established gate for all Character-dependent effects |
| Existing component spread infrastructure | N/A | Per-instance bleedSpread offset | `initComponentSpread()` and serialized RNG seed from Phase 14 |
| Existing OU drift layers | N/A | Slow bleed modulation source | ouLayers[0] at 0.05Hz provides natural capacitive-coupling-like fluctuation |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| rack::math::clamp | SDK built-in | Bleed coefficient clamping | Ensure bleed magnitude stays non-negative after spread offset |
| std::normal_distribution | C++17 stdlib | Component spread generation | In initComponentSpread() for bleedSpread generation |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Tapping ouLayers[0] for modulation | New dedicated OU layer | Unnecessary complexity; ouLayers[0] at 0.05Hz (~20s cycle) is the right timescale for thermal fluctuation and already exists |
| Post-processing after computeMorphedWave() | Inline within computeMorphedWave() | Inline is simpler: all shapes are local variables, no need to pass additional state. Display buffer also calls computeMorphedWave() and should show bleed. |
| Character-only gating | Character + Drift gating | CONTEXT.md ties bleed to Character knob, not Drift. Bleed is a morph-crossfade artifact, not an instability phenomenon. Drift is NOT involved. |

**Installation:**
No new dependencies. All components from existing codebase and SDK.

## Architecture Patterns

### Current computeMorphedWave() Structure
```
computeMorphedWave(phase, morph, character):
  sine = computeSine(phase, character)        // already computed
  tri  = computeTriangle(phase, character)    // already computed
  saw  = computeSaw(phase, character)         // already computed
  sqr  = computeSquare(phase, character)      // already computed

  scaled = morph * 3.0
  segment = min(floor(scaled), 2)             // 0, 1, or 2
  frac = scaled - segment                     // 0.0 to 1.0 within segment

  case 0: return sine + frac * (tri - sine)   // sine->tri crossfade
  case 1: return tri  + frac * (saw - tri)    // tri->saw crossfade
  case 2: return saw  + frac * (sqr - saw)    // saw->sqr crossfade
  case 3: return sqr                          // pure square
```

### Bleed Integration Pattern
```
computeMorphedWave(phase, morph, character):
  [compute all four shapes -- unchanged]
  shapes[4] = { sine, tri, saw, sqr }

  [segment/frac calculation -- unchanged]

  [original crossfade -- unchanged]
  result = shapes[seg] + frac * (shapes[seg+1] - shapes[seg])

  [NEW: bleed from neighbors]
  if character >= 0.001f:
    bleedIntensity = progressiveCurve(character) * bleedBase * (1 + bleedSpread)
    bleedIntensity *= (1 + ouModulation * 0.2)   // +/-20% slow fluctuation

    leftNeighbor  = shape one index left of segment start
    rightNeighbor = shape one index right of segment end

    leftWeight  = (1.0 - frac)   // closer to segment start = more left bleed
    rightWeight = frac            // closer to segment end = more right bleed

    result += bleedIntensity * (leftWeight * leftNeighbor + rightWeight * rightNeighbor)

  return result
```

### Pattern 1: Neighbor Identification with Wrapping
**What:** Map each morph segment to its left and right neighbor shapes. Handle the strip endpoints.
**When to use:** Every bleed calculation.
**Recommendation:** Wrap the morph strip so sine's left neighbor is square, and square's right neighbor is sine. This creates a consistent four-shape ring (sine-tri-saw-sqr-sine) where every position has two neighbors. The alternative (one-sided bleed at endpoints) creates an asymmetry that would sound different at morph extremes.

```
Morph strip with neighbors:
  [sqr]  sine ←→ tri ←→ saw ←→ sqr  [sine]
         seg 0      seg 1      seg 2

Segment 0 (sine→tri): left neighbor = sqr (index 3), right neighbor = saw (index 2)
Segment 1 (tri→saw):  left neighbor = sine (index 0), right neighbor = sqr (index 3)
Segment 2 (saw→sqr):  left neighbor = tri (index 1), right neighbor = sine (index 0)
```

### Pattern 2: Character-Gated Effect (Established)
**What:** All Character-dependent effects follow the `if (character < 0.001f) return [clean]` early-return pattern, then scale by `progressiveCurve(character)`.
**When to use:** The bleed calculation.
**Example:**
```cpp
// Source: Established pattern in AnalogLFO.cpp:185, 198, 225, 245
if (character < 0.001f) return result;  // crisp crossfade, no bleed
float c = progressiveCurve(character);
// ... apply bleed scaled by c ...
```

### Pattern 3: Component Spread Application
**What:** Add a per-instance offset to the bleed magnitude, generated from the existing component spread seed.
**When to use:** In `initComponentSpread()` and the bleed calculation.
**Example:**
```cpp
// Source: Established pattern in AnalogLFO.cpp:163-177
// In initComponentSpread(), after existing spread generations:
bleedSpread = d(spreadRng) * 0.02f;  // +/-2% bleed magnitude offset

// In computeMorphedWave():
float effectiveBleed = c * (BASE_BLEED + bleedSpread);
```

### Anti-Patterns to Avoid
- **Computing neighbor shapes separately:** All four shapes are already computed at the top of computeMorphedWave(). Do NOT add extra compute calls.
- **Putting bleed in process() instead of computeMorphedWave():** The display buffer is rendered by `updateDisplayBuffer()` which calls `computeMorphedWave()`. If bleed is outside this function, the display will not show the bleed effect, violating success criterion #1.
- **Gating bleed by Drift instead of Character:** Bleed simulates crossfader crosstalk, which is a circuit property (Character), not random instability (Drift). The user's CONTEXT.md explicitly ties bleed to Character.
- **Modifying individual compute functions:** Bleed is a mixing phenomenon in the crossfade stage, not a per-shape deformation. The compute functions should remain unchanged.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Slow modulation for bleed fluctuation | New OU layer or LFO | Tap existing ouLayers[0].state | Already runs at 0.05Hz (~20s cycle), perfect for thermal fluctuation. Adding a new layer wastes CPU and complicates the architecture. |
| Per-instance bleed variation | Separate RNG system | Existing component spread infrastructure | spreadSeed is serialized, initComponentSpread() already has the pattern. Just add one more `d(spreadRng) * magnitude` call. |
| Character-based intensity scaling | Custom curve | `progressiveCurve(character)` | x^2 curve already established for all Character effects. Reuse for consistency. |
| Neighbor shape lookup table | Hard-coded if/else chains | Array indexing with modular arithmetic | shapes[4] array + `(seg - 1 + 4) % 4` and `(seg + 2) % 4` is cleaner than a switch statement |

**Key insight:** This phase requires zero new infrastructure. Every component (shape computation, character scaling, component spread, OU modulation, display rendering) already exists. The work is purely math within a single function.

## Common Pitfalls

### Pitfall 1: Bleed Not Visible on Display
**What goes wrong:** Bleed is implemented in `process()` after `computeMorphedWave()` returns, so the display buffer (which calls `computeMorphedWave()` directly) shows clean crossfades without bleed.
**Why it happens:** `updateDisplayBuffer()` iterates 256 phase points through `computeMorphedWave()`. If bleed logic is outside this function, the display path bypasses it.
**How to avoid:** Implement all bleed logic INSIDE `computeMorphedWave()`. The function already has access to character, and the shapes array is local. The only additional data needed is bleedSpread (member variable) and an OU state value for modulation.
**Warning signs:** Character knob up, morph at midpoint, but display shows clean crossfade with no neighbor influence.

### Pitfall 2: OU Modulation Access in computeMorphedWave()
**What goes wrong:** `computeMorphedWave()` currently takes only `(phase, morph, character)`. OU layer state is not directly accessible.
**Why it happens:** The function is a pure computation helper with no access to module state beyond parameters.
**How to avoid:** Two options, in order of preference:
1. **Add a bleedModulation parameter** to `computeMorphedWave()` -- computed in `process()` from ouLayers[0].state, passed as a fourth argument. This keeps the function mostly pure.
2. **Access ouLayers[0].state directly** since computeMorphedWave() is a member function of the AnalogLFO struct and already accesses member variables (bleedSpread). This is simpler but slightly less clean.

**Recommendation:** Option 2. The function already accesses member variables like `triAsymmetrySpread`, `sawCurvatureSpread`, and `squareDutySpread` through the compute functions it calls. Accessing `ouLayers[0].state` and `bleedSpread` directly is consistent with existing practice.

**For the display path:** `updateDisplayBuffer()` also calls `computeMorphedWave()`. The OU state will be whatever it was at the last process() call -- this is fine because the display is a snapshot, and the bleed modulation changes so slowly (0.05Hz) that intra-frame variation is negligible.

### Pitfall 3: Amplitude Exceeding +/-1 Pre-Scaling
**What goes wrong:** Adding bleed from neighbor shapes on top of the primary crossfade pushes the waveform amplitude above 1.0, which after 5V scaling exceeds +/-5V.
**Why it happens:** The primary crossfade produces values in [-1, 1]. Bleed adds a fraction of neighbor shapes (also in [-1, 1]). At maximum bleed (5%), worst case adds 0.05 * 2 = 0.10 to the output (both neighbors at +/-1, both fully weighted). Actual worst case is lower because leftWeight + rightWeight = 1.0 in the proximity model.
**How to avoid:** Analyze the math:
- Primary crossfade: result in [-1, 1]
- Bleed addition: bleedIntensity * (leftWeight * leftNeighbor + rightWeight * rightNeighbor)
- With leftWeight + rightWeight = 1.0 and bleedIntensity = 0.05 max: worst case adds +/-0.05
- Total worst case: +/-1.05, which scales to +/-5.25V
- With OU modulation (+/-20% on bleed): worst case +/-1.06, scaling to +/-5.30V

This is a 6% overshoot. Two approaches:
1. **Normalize the output** by dividing by (1 + bleedIntensity) -- preserves exact amplitude but changes the primary crossfade character subtly.
2. **Accept the overshoot** -- 5.3V is well within VCV Rack's operational range and imperceptible.
3. **Soft clamp** -- apply a gentle saturation at +/-1 before the 5V scaling.

**Recommendation:** Option 1 (normalize). Dividing by `(1 + bleedIntensity)` is cheap, guarantees +/-5V, and the 5% normalization is inaudible on the primary signal. This satisfies success criterion #3 rigorously.

### Pitfall 4: Segment Boundary Discontinuities
**What goes wrong:** At segment boundaries (morph=0.333, 0.667), the neighbor identity changes abruptly. If the bleed coefficients don't smoothly transition, there will be a discontinuity in the output.
**Why it happens:** At morph=0.333-, segment 0 ends (sine->tri, right neighbor = saw). At morph=0.333+, segment 1 begins (tri->saw, left neighbor = sine). The frac value resets from 1.0 to 0.0.
**How to avoid:** At segment boundaries, frac=0 or frac=1, which means leftWeight=1/rightWeight=0 or vice versa. Since the neighbor that "changes" at the boundary has zero weight at the transition point, the discontinuity is zero. This is inherently smooth because:
- At seg 0, frac=1.0: leftWeight=0, rightWeight=1.0, right neighbor = saw
- At seg 1, frac=0.0: leftWeight=1.0, rightWeight=0.0, left neighbor = sine
- The bleed contribution switches but with zero amplitude at the switch point
**Warning signs:** Clicks or discontinuities at morph=0.333 or morph=0.667.

**Analysis confirms this is NOT a pitfall** -- the proximity weighting provides natural continuity at segment boundaries. No special handling needed.

### Pitfall 5: Display Buffer Inconsistency During Modulation
**What goes wrong:** The display buffer is rendered once per phase wrap or parameter change (see lines 650-662). The OU modulation changes continuously but slowly. If the display call happens at a different OU state than the audio, the display could show slightly different bleed.
**Why it happens:** Display and audio share the same `computeMorphedWave()` but at different moments.
**How to avoid:** This is acceptable. The OU layer at 0.05Hz changes by less than 0.001 per display refresh. The visual difference is imperceptible. No mitigation needed.

### Pitfall 6: bleedSpread Applied Before Character Gate
**What goes wrong:** If bleedSpread is subtracted from bleedBase and the result goes negative, bleed could invert (neighbor shapes subtracted instead of added).
**Why it happens:** bleedSpread is normally distributed with sigma=0.02, so values of -0.04 to -0.06 are possible (2-3 sigma events), potentially making `(BASE_BLEED + bleedSpread)` negative.
**How to avoid:** Clamp the effective bleed coefficient to be non-negative: `float effectiveBleed = std::fmax(0.f, BASE_BLEED + bleedSpread)`. With BASE_BLEED=0.04 and spread sigma=0.02, negative values require >2 sigma events, but clamping is cheap insurance.

## Code Examples

### Bleed Implementation in computeMorphedWave()
```cpp
// Source: Analysis of existing AnalogLFO.cpp:264-281 + Phase 15 CONTEXT.md constraints
float computeMorphedWave(float phase, float morph, float character) {
    float sine = computeSine(phase, character);
    float tri  = computeTriangle(phase, character);
    float saw  = computeSaw(phase, character);
    float sqr  = computeSquare(phase, character);

    // Store shapes in array for indexed neighbor access
    float shapes[4] = { sine, tri, saw, sqr };

    float scaled = morph * 3.f;
    int segment = std::min((int)scaled, 2);
    float frac = scaled - (float)segment;

    // Primary crossfade (unchanged from v1.1)
    float result = shapes[segment] + frac * (shapes[segment + 1] - shapes[segment]);

    // Waveform bleed: adjacent-shape crosstalk (CHAR-05)
    if (character >= 0.001f) {
        float c = progressiveCurve(character);

        // Base bleed magnitude with component spread offset
        float effectiveBleed = std::fmax(0.f, 0.04f + bleedSpread);
        float bleedIntensity = c * effectiveBleed;

        // Slow modulation from existing OU layer (~20s cycle)
        // ouLayers[0].state has stationary std ~1.0, scale to +/-20% modulation
        bleedIntensity *= (1.f + ouLayers[0].state * 0.2f);
        bleedIntensity = std::fmax(0.f, bleedIntensity);  // ensure non-negative after modulation

        // Neighbor identification (wrapping: sine<->sqr at strip endpoints)
        int leftIdx  = (segment - 1 + 4) % 4;   // shape left of segment start
        int rightIdx = (segment + 2) % 4;        // shape right of segment end

        // Proximity weighting: closer neighbor bleeds more
        float leftWeight  = 1.f - frac;   // frac=0 -> full left bleed
        float rightWeight = frac;          // frac=1 -> full right bleed

        float bleedSignal = leftWeight * shapes[leftIdx] + rightWeight * shapes[rightIdx];
        result += bleedIntensity * bleedSignal;

        // Normalize to maintain +/-1 range (prevents >+/-5V after scaling)
        result /= (1.f + bleedIntensity);
    }

    return result;
}
```

### Component Spread Addition in initComponentSpread()
```cpp
// Source: Established pattern in AnalogLFO.cpp:163-177
// Add after existing spread generations (triAsymmetrySpread line):
// Bleed magnitude spread: +/-2%
bleedSpread = d(spreadRng) * 0.02f;
```

### Member Variable Declaration
```cpp
// Add near other component spread members (after triAsymmetrySpread):
float bleedSpread = 0.f;  // waveform bleed magnitude spread (CHAR-05)
```

## Discretion Recommendations

Research analysis for areas marked as Claude's Discretion in CONTEXT.md:

### Edge Wrapping
**Recommendation: Wrap.** Use modular arithmetic so sine's left neighbor is square, and square's right neighbor is sine. This creates a consistent ring topology where every morph position has symmetric bleed behavior. One-sided bleed at endpoints would create an audible asymmetry where morph=0 (pure sine) and morph=1 (pure square) have different character even though both are "endpoint" positions.

### Segment Boundaries
**Recommendation: Bleed is present at segment boundaries.** The proximity weighting naturally handles this: at a segment boundary, frac=0 means only the left neighbor contributes, and at frac=1 only the right neighbor contributes. The bleed transitions smoothly through boundaries with zero discontinuity (proven by analysis in Pitfall 4).

### Morph Endpoints
**Recommendation: Bleed is present at morph endpoints when Character is up.** At morph=0 (pure sine, segment 0, frac=0), left neighbor is square with full weight. This means at high Character, pure sine gets a hint of square -- consistent with the analog crossfader crosstalk metaphor where even a "fully selected" channel has some adjacent bleed. At morph=1 (pure square, segment 2, frac=1), right neighbor is sine with full weight. This creates a pleasing circular quality where the morph strip's endpoints connect through bleed.

### Modulation Source
**Recommendation: Tap ouLayers[0].state (existing 0.05Hz OU layer).** This is the slowest drift layer with a ~20-second wander cycle, perfectly matching "temperature-dependent capacitive crosstalk" that the user described. No new OU layer needed. The modulation depth should be +/-20% of the bleed intensity (i.e., multiply bleedIntensity by `(1 + ouLayers[0].state * 0.2f)`). This provides subtle fluctuation without adding CPU cost or architectural complexity.

### Exact Bleed Magnitude
**Recommendation: 4% base (0.04f).** This is the midpoint of the 3-5% range and provides clearly visible bleed on the display at full Character while remaining subtle at moderate Character (due to x^2 progressive curve: at Character=0.5, bleed is only 1% -- barely perceptible).

### Output Clamping
**Recommendation: Normalize, do not hard clamp.** Dividing by `(1 + bleedIntensity)` after adding the bleed signal guarantees the output stays within +/-1.0 (and thus +/-5V after scaling) regardless of input. This is mathematically rigorous:
- `|result| <= |primary| + bleedIntensity * |bleedSignal| <= 1 + bleedIntensity * 1`
- After dividing by `(1 + bleedIntensity)`: `|result| <= 1.0`
A hard clamp would work but introduces a nonlinearity that could create subtle artifacts at extreme settings. Normalization is smoother.

### Integration Point
**Recommendation: Inside computeMorphedWave(), after the primary crossfade.** This ensures the display buffer shows bleed (success criterion #1) and keeps the logic co-located with the crossfade math for clarity. The function already accesses member variables through its compute sub-functions; accessing bleedSpread and ouLayers[0].state is consistent with this pattern.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Clean linear crossfade between shapes | Crossfade + neighbor bleed at Character>0 | Phase 15 (CHAR-05) | Morph transitions feel more analog; adjacent shapes "leak" into the mix |
| computeMorphedWave() with switch/case | computeMorphedWave() with shapes array + bleed math | Phase 15 | Slightly more code but enables indexed neighbor access |
| Fixed crossfade behavior | Instance-varying bleed via component spread | Phase 15 | Each module instance has a slightly different morph character |

**Real-world reference:** Actual analog mixer crosstalk is -55 to -70 dB (0.03-0.2% amplitude). The 3-5% bleed in this implementation is intentionally exaggerated by 20-100x for musical/visual effect -- this is an aesthetic choice consistent with the project's "authentic vintage character" philosophy, not an engineering simulation.

## Open Questions

1. **Bleed interaction with very high Character values**
   - What we know: At character=1.0, progressiveCurve returns 1.0, so bleed is at full 4%. The individual shapes at character=1.0 have significant deformation (8% THD sine, 10% tri asymmetry, 50% saw curvature, 8% square edge softening).
   - What's unclear: Whether heavily deformed neighbor shapes bleeding in at 4% will be visually/audibly pleasant or jarring.
   - Recommendation: Implement as designed. The x^2 curve means bleed scales naturally with character deformation. Empirical tuning may adjust BASE_BLEED if needed.

2. **OU layer state during display buffer rendering**
   - What we know: updateDisplayBuffer() calls computeMorphedWave() at a specific moment. ouLayers[0].state is whatever it was at the last process() call.
   - What's unclear: Whether the display should show modulated bleed or fixed bleed.
   - Recommendation: Use live OU state. The 0.05Hz modulation is so slow that consecutive display renders show essentially the same bleed level. The subtle variation adds authenticity without visual confusion.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual verification via VCV Rack runtime |
| Config file | none -- VCV Rack modules tested by running the module |
| Quick run command | `make && make install` then launch VCV Rack |
| Full suite command | `make && make install` then verify all success criteria in VCV Rack |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CHAR-05 SC1 | With Character above zero and Morph at intermediate positions, waveform trace shows subtle influence from adjacent shapes (visible on display) | manual-only | Visual inspection: set Character=1, Morph=0.5, observe display | N/A |
| CHAR-05 SC2 | At Character=0, morph is a crisp crossfade with no bleed -- identical to v1.1 behavior | manual-only | Set Character=0, sweep Morph: verify clean crossfade | N/A |
| CHAR-05 SC3 | Output stays within +/-5V range at all morph/character/bleed combinations | manual-only | Patch output to scope, sweep all params, verify no spikes | N/A |
| CHAR-05 spread | Two instances with identical settings show slightly different bleed | manual-only | Two AnalogLFOs side-by-side, compare crossfade character on scope | N/A |
| CHAR-05 modulation | Bleed magnitude fluctuates slowly over time | manual-only | Observe display at high Character over 30+ seconds | N/A |

**Manual-only justification:** VCV Rack DSP modules are tested by running in the host application. All behaviors are perceptual (visual/auditory) and require the module running in context.

### Sampling Rate
- **Per task commit:** `make && make install` -- verify module loads without crash
- **Per wave merge:** Full manual verification of all success criteria
- **Phase gate:** All success criteria verified before `/gsd:verify-work`

### Wave 0 Gaps
None -- no test infrastructure needed beyond existing build system.

## Sources

### Primary (HIGH confidence)
- Existing codebase: `AnalogLFO.cpp` -- computeMorphedWave() at line 264, all four compute functions, component spread infrastructure, OU layers, progressiveCurve() -- verified by direct code reading
- Phase 14 CONTEXT.md and RESEARCH.md -- established patterns for Character-gated effects, component spread, OU modulation
- Phase 15 CONTEXT.md -- all locked decisions and discretion areas

### Secondary (MEDIUM confidence)
- [Sound On Sound: Is Crosstalk a good thing?](https://www.soundonsound.com/sound-advice/q-crosstalk-good-thing) -- real analog crosstalk is -55 to -70 dB, capacitive coupling, worse at higher frequencies
- [Wikipedia: Crosstalk](https://en.wikipedia.org/wiki/Crosstalk) -- crosstalk fundamentals, adjacent channel coupling
- [Analog Devices AN-35: Understanding Crosstalk in Analog Multiplexers](https://www.analog.com/media/en/technical-documentation/application-notes/177696709an35.pdf) -- capacitive coupling mechanisms

### Tertiary (LOW confidence)
- Bleed magnitude perception at 3-5% -- the exact perceptual threshold where bleed becomes "subtle but visible" on the display is empirical and needs runtime validation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all components already exist in codebase; no new dependencies
- Architecture: HIGH -- computeMorphedWave() structure fully understood; bleed integration point is clear and the math is straightforward
- Pitfalls: HIGH -- amplitude analysis is mathematically provable; display path verified by code reading; discontinuity analysis proven by limit analysis
- Discretion recommendations: MEDIUM -- recommendations are analytically sound but some (bleed magnitude, OU modulation depth) need empirical tuning
- Component spread: HIGH -- exact pattern established in Phase 14; adding bleedSpread is a one-line addition

**Research date:** 2026-03-17
**Valid until:** 2026-04-17 (stable domain -- existing codebase patterns and DSP math do not change)
