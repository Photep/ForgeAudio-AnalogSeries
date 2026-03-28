# Phase 18: PWM DSP Extension - Research

**Researched:** 2026-03-28
**Domain:** DSP waveform synthesis -- pulse wave generation with analog character modeling
**Confidence:** HIGH

## Summary

Phase 18 extends the existing 4-shape morph engine (sine-tri-saw-square) to 5 shapes by adding a variable-width pulse waveform. The existing codebase is exceptionally well-structured for this extension: each shape has a dedicated `compute*()` function, shapes are stored in an indexed array, and the morph engine uses modular arithmetic for its bleed ring topology. The changes are surgical -- a new `computePulse()` function modeled after `computeSquare()`, and a handful of constant changes (4->5, 3->4, 2->3) in `computeMorphedWave()`.

The user has explicitly dropped backward compatibility (D-02), which dramatically simplifies the implementation. The morph range remaps from `morph * 3.0` (4 shapes, 3 segments) to `morph * 4.0` (5 shapes, 4 segments). Existing patches with morph values in [0, 1] will produce different shapes at the same knob position -- this is an accepted breaking change.

**Primary recommendation:** Implement `computePulse(phase, character, duty)` using the same tanh edge softening pattern as `computeSquare()`, with duty cycle derived from the morph position within the pulse segment. Add a `pulseEdgeSpread` component spread variable following the established pattern. Update `computeMorphedWave()` constants from 4-shape to 5-shape. Total change: ~40 lines of new code, ~10 lines of modified code.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Even 5-shape rescale -- `morph * 4.0` gives 4 segments across [0, 1.0], each shape getting 20% of the knob range
- **D-02:** WAVE-05 (backward compatibility) is DROPPED -- existing patches will shift morph positions. Breaking change accepted.
- **D-03:** Morph order: Sine -> Triangle -> Saw -> Square -> Pulse (natural harmonic progression, smooth to sharp to narrow)
- **D-04:** Full 5-shape wrapping ring: sine-tri-saw-sqr-pulse-sine (`% 5` modular arithmetic). Pulse neighbors sine, consistent with existing ring pattern.
- **D-05:** Tanh edge softening, same approach as `computeSquare` -- at character=0 crisp digital pulse, at character=1 rounded analog edges
- **D-06:** Component spread affects edge softening intensity only, NOT duty cycle. Duty cycle stays where morph puts it.
- **D-07:** Duty ranges from 50% (at square->pulse boundary) to 5% (at morph=1.0) -- classic PWM territory
- **D-08:** Linear duty mapping: `duty = 0.50 - 0.45 * frac` where frac is the position within the pulse segment

### Claude's Discretion
- Pulse generation implementation details (threshold comparison, phase-based construction)
- How to structure `computePulse()` function relative to existing per-shape compute functions
- Any anti-aliasing or edge-case handling for extreme narrow pulse widths at LFO rates

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WAVE-01 | Morph sweep extends past square into variable-width pulse (Sine -> Tri -> Saw -> Square -> Pulse) | 5-shape rescale via `morph * 4.0`, new `computePulse()` function, shapes array expanded from [4] to [5] |
| WAVE-02 | Pulse duty cycle ranges from 50% (square) to 5% (narrow pulse) across morph range | Linear duty mapping `duty = 0.50 - 0.45 * frac` computed from morph position within pulse segment |
| WAVE-03 | Character knob applies analog deformation to pulse shape (tanh edge softening, component spread) | Tanh edge softening pattern reused from `computeSquare()`, new `pulseEdgeSpread` component spread variable |
| WAVE-04 | Waveform bleed ring wraps to 5 shapes (pulse neighbors sine) | `% 4` becomes `% 5` in bleed neighbor identification; ring order: sine-tri-saw-sqr-pulse-sine |
| WAVE-05 | ~~Existing morph positions preserved for backward compatibility~~ | **DROPPED per D-02.** Breaking change accepted. No migration code needed. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | 2.x | Plugin framework, DSP primitives | Required platform |
| C++17 | -- | Language standard | Project constraint |

### Supporting
No additional libraries needed. All DSP is hand-written using `<cmath>` functions (`std::tanh`, `std::fabs`, `std::fmax`, `std::sin`, `std::cos`) consistent with the existing codebase.

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Hand-written tanh pulse | PolyBLEP anti-aliased pulse | Overkill for LFO rates (<20Hz); adds complexity for no audible benefit at sub-audio frequencies |
| Phase threshold comparison | Wavetable lookup | Unnecessary complexity; direct computation is fast and exact at LFO rates |

## Architecture Patterns

### Existing Code Structure (unchanged)
```
src/AnalogLFO.cpp
  Module struct:
    - Component spread variables (member fields, lines 146-153)
    - initComponentSpread() (lines 196-212)
    - computeSine/computeTriangle/computeSaw/computeSquare (lines 218-297)
    - computeMorphedWave() (lines 299-343)
    - updateDisplayBuffer() (lines 345-359)
    - dataToJson/dataFromJson (lines 579-604)
    - process() (line 606+)
```

### Pattern 1: Per-Shape Compute Function
**What:** Each waveform has a dedicated `compute*()` function taking `(float phase, float character)` and returning a float in [-1, 1]. Character=0 returns the pure digital shape; higher values add analog deformation via `progressiveCurve()`.
**When to use:** Always -- this is the established pattern for all 4 existing shapes.
**Example (from existing `computeSquare`, lines 276-297):**
```cpp
float computeSquare(float phase, float character) {
    float sqr = (phase < 0.5f) ? 1.f : -1.f;
    if (character < 0.001f) return sqr;
    float c = progressiveCurve(character);
    float duty = 0.5f + c * (0.04f + squareDutySpread);
    float edgeWidth = c * 0.08f;
    float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);
    float center = duty * 0.5f;
    float halfWidth = duty * 0.5f;
    float d = phase - center;
    if (d > 0.5f) d -= 1.f;
    if (d < -0.5f) d += 1.f;
    float dist = halfWidth - std::fabs(d);
    float analog = std::tanh(sharpness * dist);
    return sqr + c * (analog - sqr);
}
```

### Pattern 2: Component Spread
**What:** Each shape has per-instance random offsets (normally distributed) seeded from a deterministic `spreadSeed`. Offsets are small (+/-1-3%) and applied to character-dependent parameters.
**When to use:** Every new shape must add its spread variable to `initComponentSpread()`.
**Example (from lines 207-211):**
```cpp
sawCurvatureSpread = d(spreadRng) * 0.02f;    // +/-2%
squareDutySpread = d(spreadRng) * 0.01f;       // +/-1%
triAsymmetrySpread = d(spreadRng) * 0.015f;    // +/-1.5%
bleedSpread = d(spreadRng) * 0.02f;            // +/-2%
```

### Pattern 3: Morph Crossfade with Bleed Ring
**What:** `computeMorphedWave()` computes all shapes into an array, selects a segment pair for linear crossfade, then optionally adds proximity-weighted neighbor bleed from the wrapping ring.
**When to use:** This is the single morph function -- modifications go here.
**Key constants to update:**
```
shapes[4] -> shapes[5]
morph * 3.f -> morph * 4.f
std::min((int)scaled, 2) -> std::min((int)scaled, 3)
% 4 -> % 5 (two occurrences)
```

### Anti-Patterns to Avoid
- **Separate duty cycle knob/CV:** Out of scope (see REQUIREMENTS.md "Out of Scope"). Duty cycle comes from morph position only.
- **Anti-aliasing for pulse:** Unnecessary at LFO rates. PolyBLEP or BLIT would add CPU cost with zero audible benefit below 20Hz. Deferred to VCO module (VCO-04).
- **Modifying `computeSquare()`:** Square wave remains at 50% duty. The pulse is a NEW shape, not a modification of square. The morph crossfade between square and pulse naturally transitions from 50% to narrower duty cycles.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Edge softening | Custom sigmoid | `std::tanh()` with sharpness parameter | Already proven in `computeSquare()`, mathematically clean, CPU-efficient |
| Progressive character curve | Custom curve function | Existing `progressiveCurve()` | Already used by all 4 shapes; maintains consistent character feel |
| Per-instance variation | Manual random offsets | Existing `initComponentSpread()` pattern | Deterministic from seed, already serialized, proven |
| Wrapping neighbor access | Custom boundary checks | Modular arithmetic `% 5` | Existing pattern, clean wrap at boundaries |

**Key insight:** This phase reuses every established pattern. The only new code is `computePulse()` itself and the 5-shape constants in `computeMorphedWave()`.

## Common Pitfalls

### Pitfall 1: Duty Cycle at Extreme Narrow Widths
**What goes wrong:** At duty=5% (morph=1.0), the positive pulse is only 5% of the cycle. The tanh edge softening width could exceed the pulse width itself, causing the pulse to never reach full +1 amplitude.
**Why it happens:** `edgeWidth = c * 0.08f` at full character means ~8% transition zone, but the pulse high region is only 5% wide.
**How to avoid:** Scale `edgeWidth` relative to `min(duty, 1.0 - duty)` so edges never exceed the narrower of the two regions. E.g., `edgeWidth = c * std::fmin(0.08f, duty * 0.8f)`.
**Warning signs:** At full character + full morph, the waveform display shows a rounded bump instead of a narrow pulse; amplitude drops noticeably.

### Pitfall 2: Discontinuity at Square-Pulse Boundary
**What goes wrong:** At the exact boundary between square (segment 3) and pulse (segment 4), if the pulse at frac=0 doesn't exactly match a 50% duty square, there's a timbral click when sweeping morph.
**Why it happens:** `computeSquare()` has its own slight duty offset via `squareDutySpread` and `0.04f` character-dependent shift. If `computePulse()` at frac=0 assumes exactly 50% duty, the crossfade has a shape mismatch.
**How to avoid:** At frac=0 (morph entering pulse segment), duty=0.50 per D-08. The square at full morph into segment 3 already crossfades to segment 4. The linear crossfade in `computeMorphedWave()` handles any slight mismatch smoothly -- but verify audibly that the transition is click-free.
**Warning signs:** A pop or timbral shift when slowly sweeping morph through the 0.80 region (square-to-pulse boundary with 5 shapes each getting 0.20 range).

### Pitfall 3: Pulse-Sine Bleed Ring Wrap
**What goes wrong:** When morph is in the pulse region (segment 4) and bleed is active, the right neighbor wraps to sine (index 0). If sine and pulse have very different amplitudes or DC offsets at the same phase, the bleed sounds unmusical.
**Why it happens:** Sine is smooth and symmetric; a narrow pulse is peaky and asymmetric. Their crosstalk can sound harsh.
**How to avoid:** This is by design (D-04 mandates the wrapping ring). The bleed magnitude is small (4% base) and character-gated, so the effect is subtle. Verify it sounds acceptable by ear, especially at narrow duty + high character + high morph.
**Warning signs:** Harsh or metallic quality when morph is near 1.0 with character above 0.5.

### Pitfall 4: Component Spread Ordering Sensitivity
**What goes wrong:** Adding a new `d(spreadRng)` call in `initComponentSpread()` changes the sequence for all subsequent calls, altering existing spread values for other shapes.
**Why it happens:** The normal distribution draws are sequential from a seeded PRNG. Inserting a draw before existing ones shifts all downstream values.
**How to avoid:** Append the new `pulseEdgeSpread = d(spreadRng) * ...` AFTER all existing spread draws (after `bleedSpread` on line 211). This preserves all existing spread values for patches that reload.
**Warning signs:** Existing patches sound slightly different after the update, even outside the pulse range. (Note: since D-02 drops backward compat, this is less critical, but still good practice to minimize unintended changes.)

### Pitfall 5: Display Buffer Shows Pulse Automatically
**What goes wrong:** Not a problem -- `updateDisplayBuffer()` calls `computeMorphedWave()` which will automatically render the pulse shape once the engine is extended. No display code changes needed for Phase 18.
**Why it's worth noting:** Confirms that this phase is truly DSP-only. Display pills, layout, and rendering are not affected.

## Code Examples

### computePulse() -- Recommended Implementation

Based on the established `computeSquare()` pattern, adapted for variable duty cycle:

```cpp
// Source: Derived from existing computeSquare() pattern (lines 276-297)
// duty parameter: 0.50 (at square boundary) to 0.05 (at morph=1.0)
float computePulse(float phase, float character, float duty) {
    // Digital: +1 for phase < duty, -1 for phase >= duty
    float pulse = (phase < duty) ? 1.f : -1.f;
    if (character < 0.001f) return pulse;
    float c = progressiveCurve(character);

    // Tanh edge softening (D-05): same approach as computeSquare
    // Scale edge width to avoid exceeding narrow pulse region
    float maxEdge = std::fmin(duty, 1.f - duty) * 0.8f;
    float edgeWidth = c * std::fmin(0.08f, maxEdge);
    float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);

    // Component spread affects edge softening intensity (D-06)
    sharpness *= (1.f + pulseEdgeSpread);

    // Soft pulse: +1 region [0, duty], -1 region [duty, 1]
    float center = duty * 0.5f;
    float halfWidth = duty * 0.5f;
    float d = phase - center;
    if (d > 0.5f) d -= 1.f;
    if (d < -0.5f) d += 1.f;
    float dist = halfWidth - std::fabs(d);
    float analog = std::tanh(sharpness * dist);

    // Crossfade: prevents snap at low character values
    return pulse + c * (analog - pulse);
}
```

### Updated computeMorphedWave() -- Key Changes

```cpp
// Source: Modification of existing computeMorphedWave() (lines 299-343)
float computeMorphedWave(float phase, float morph, float character) {
    float sine = computeSine(phase, character);
    float tri  = computeTriangle(phase, character);
    float saw  = computeSaw(phase, character);
    float sqr  = computeSquare(phase, character);

    // Derive pulse duty from morph position within pulse segment
    // D-01: morph * 4.0 gives 5 shapes across [0, 1]
    float scaled = morph * 4.f;
    int segment = std::min((int)scaled, 3);  // 0-3 (4 segments for 5 shapes)
    float frac = scaled - (float)segment;

    // D-08: duty = 0.50 - 0.45 * frac (only meaningful when segment == 4's frac)
    // Compute pulse with duty derived from full morph position in pulse region
    float pulseFrac = std::fmax(0.f, morph * 4.f - 3.f);  // 0 at square/pulse boundary, 1 at morph=1
    float pulseDuty = 0.50f - 0.45f * std::fmin(pulseFrac, 1.f);  // D-07: 50% to 5%
    float pls  = computePulse(phase, character, pulseDuty);

    // D-03: Sine -> Tri -> Saw -> Square -> Pulse
    float shapes[5] = { sine, tri, saw, sqr, pls };

    // Primary crossfade (unchanged logic)
    float result = shapes[segment] + frac * (shapes[segment + 1] - shapes[segment]);

    // Waveform bleed: adjacent-shape crosstalk
    if (character >= 0.001f) {
        float c = progressiveCurve(character);
        float effectiveBleed = std::fmax(0.f, 0.04f + bleedSpread);
        float bleedIntensity = c * effectiveBleed;
        bleedIntensity *= (1.f + ouLayers[0].state * 0.2f);
        bleedIntensity = std::fmax(0.f, bleedIntensity);

        // D-04: wrapping ring sine-tri-saw-sqr-pulse-sine (% 5)
        int leftIdx  = (segment - 1 + 5) % 5;
        int rightIdx = (segment + 2) % 5;

        float leftWeight  = 1.f - frac;
        float rightWeight = frac;
        float bleedSignal = leftWeight * shapes[leftIdx] + rightWeight * shapes[rightIdx];
        result += bleedIntensity * bleedSignal;
        result /= (1.f + bleedIntensity);
    }

    return result;
}
```

### Component Spread Addition

```cpp
// Member variable (add after bleedSpread, line 153)
float pulseEdgeSpread = 0.f;   // pulse edge softening spread

// In initComponentSpread() (add AFTER bleedSpread line 211)
pulseEdgeSpread = d(spreadRng) * 0.02f;   // +/-2% edge softening variation
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| 4-shape morph (sine-tri-saw-sqr) | 5-shape morph (sine-tri-saw-sqr-pulse) | This phase | Richer timbral range from single morph knob |
| morph * 3.0 scaling | morph * 4.0 scaling | This phase | Breaking change (D-02): existing morph positions shift |
| 4-shape bleed ring (% 4) | 5-shape bleed ring (% 5) | This phase | Pulse wraps to sine, consistent ring topology |

**Not applicable to this phase:**
- Anti-aliasing techniques (PolyBLEP, BLIT, PTR) -- deferred to VCO module (VCO-04) since they serve no purpose at LFO rates
- Wavetable synthesis -- not applicable to this analog-modeling approach

## Open Questions

1. **Pulse amplitude at extreme narrow duty + full character**
   - What we know: The tanh sharpness scaling (Pitfall 1 mitigation) should prevent amplitude collapse
   - What's unclear: Whether the exact `0.8f` scaling factor produces the best subjective result at duty=5%, character=1.0
   - Recommendation: Implement with `0.8f` factor, tune by ear during verification. This is a single constant if adjustment is needed.

2. **Pulse duty at frac=0 vs square at frac=1**
   - What we know: At the square-pulse crossfade boundary, square has character-dependent duty of `0.5 + c * (0.04 + squareDutySpread)` while pulse starts at exactly 0.50 duty
   - What's unclear: Whether the slight mismatch (up to ~4% at full character) causes audible artifacts during crossfade
   - Recommendation: The linear crossfade smooths this. If it sounds bad, pulse could start at a character-dependent duty matching square's endpoint, but this likely unnecessary.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| VCV Rack 2 SDK | Build/compile | Yes | 2.x (present at ../Rack-SDK) | -- |
| make (GNU) | Build system | Yes | 3.81 | -- |
| C++ compiler | Compilation | Yes | Apple clang (via Rack SDK) | -- |

**Missing dependencies:** None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None -- no test infrastructure exists in project |
| Config file | None |
| Quick run command | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4` |
| Full suite command | Build + manual verification in VCV Rack |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WAVE-01 | 5-shape morph sweep produces sine->tri->saw->sqr->pulse | manual | Build, load in VCV Rack, sweep morph 0->1 | N/A |
| WAVE-02 | Pulse duty 50% to 5% across morph range | manual | Visually verify on display while sweeping morph in pulse region | N/A |
| WAVE-03 | Character knob softens pulse edges | manual | Turn character while morph is in pulse region, observe display | N/A |
| WAVE-04 | Bleed ring wraps 5 shapes | manual | Set morph near 1.0, turn character up, listen for sine bleed | N/A |
| WAVE-05 | DROPPED | -- | -- | -- |

### Sampling Rate
- **Per task commit:** `make -j4` (compilation succeeds = no syntax/type errors)
- **Per wave merge:** Build + load in VCV Rack for manual shape verification
- **Phase gate:** Full 5-shape sweep verification in VCV Rack before `/gsd:verify-work`

### Wave 0 Gaps
None required -- this is a C++ VCV Rack plugin with no automated test framework. Validation is compilation + manual testing in the host application. The `make` build system provides compile-time verification.

## Sources

### Primary (HIGH confidence)
- **Existing codebase:** `src/AnalogLFO.cpp` lines 276-297 (`computeSquare`), 299-343 (`computeMorphedWave`), 196-212 (`initComponentSpread`) -- direct inspection of code to be modified
- **CONTEXT.md:** All 8 implementation decisions (D-01 through D-08) reviewed and incorporated
- **VCV Rack Fundamental LFO:** `src/LFO.cpp` -- confirmed standard pulse generation uses phase threshold comparison with variable duty cycle

### Secondary (MEDIUM confidence)
- **Bogaudio Modules:** PWM implementation uses knob/CV control of pulse width on square wave -- validates the phase-comparison approach as standard in the VCV ecosystem
- **CCRMA BLIT paper:** Confirms anti-aliasing is unnecessary for sub-audio LFO rates

### Tertiary (LOW confidence)
- None -- all findings verified against primary sources

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all code patterns directly observed in existing codebase
- Architecture: HIGH -- extending established patterns, no new architectural decisions
- Pitfalls: HIGH -- derived from direct code analysis of edge cases in existing implementation
- Implementation approach: HIGH -- `computePulse()` is a well-understood adaptation of `computeSquare()`

**Research date:** 2026-03-28
**Valid until:** Indefinite -- DSP fundamentals and existing codebase patterns are stable
