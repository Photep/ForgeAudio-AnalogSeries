# Requirements: Analog Series LFO v1.1 Clock Sync

**Defined:** 2026-03-07
**Core Value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.

## v1.1 Requirements

Requirements for clock sync release. Each maps to roadmap phases.

### Clock Input

- [ ] **CLK-01**: LFO accepts clock trigger input via CLK jack
- [ ] **CLK-02**: Edge detection uses SchmittTrigger with VCV standard thresholds (0.1V/1.0V)
- [ ] **CLK-03**: Clock period measured with float timer and EMA smoothing (alpha ~0.3)
- [ ] **CLK-04**: Clock-loss timeout (3x smoothed period) reverts to free-running mode
- [ ] **CLK-05**: Outlier rejection filters tempo jumps exceeding 3x current period
- [ ] **CLK-06**: First clock pulse resets phase without setting frequency (waits for second edge)

### Rate & Ratios

- [ ] **RATE-01**: Rate knob sets frequency directly when CLK is disconnected (identical to v1.0)
- [ ] **RATE-02**: Rate knob selects clock division/multiplication ratio when CLK is connected
- [ ] **RATE-03**: 15 discrete snapped ratios: /16, /8, /6, /4, /3, /2, /1.5, x1, x1.5, x2, x3, x4, x6, x8, x16
- [ ] **RATE-04**: Phase resets to 0 on clock edge, with division-aware counting for /N ratios
- [ ] **RATE-05**: Anti-click crossfade (2-5ms cosine) on output after phase reset
- [ ] **RATE-06**: Custom ParamQuantity tooltip shows ratio label (e.g., "x4 (synced)") in clocked mode

### Display & Panel

- [ ] **DISP-01**: Panel SVG updated with CLK jack and label
- [ ] **DISP-02**: Waveform display shows "SYNC" badge when clocked
- [ ] **DISP-03**: Display shows current division ratio label
- [ ] **DISP-04**: Drift authority reduced in clocked mode (~2% vs 7.5% free-running)
- [ ] **DISP-05**: Smooth frequency slew during clock-to-free and free-to-clock transitions
- [ ] **DISP-06**: Display shows BPM calculated from clock source and the currently selected rate divider

## Future Requirements

### Deferred from v1.1

- **CLK-F01**: CV control of division ratio
- **CLK-F02**: Separate RESET jack (independent from CLK)
- **CLK-F03**: Animated sync badge (clock-pulse flash)
- **CLK-F04**: Phase offset knob/CV
- **CLK-F05**: Swing/shuffle control

### Deferred from v1.0

- **LFO-F01**: FM input
- **LFO-F02**: Phase jitter, DC offset drift, pitch slew, component spread
- **LFO-F03**: Waveform bleed in morph transitions

## Out of Scope

| Feature | Reason |
|---------|--------|
| PLL-based tracking | Overkill for LFO rates; simple edge measurement + EMA is sufficient |
| External clock libraries | VCV SDK built-ins cover all needs |
| Tap tempo button | CLK input IS tap tempo when patched from a manual trigger |
| Serialization of clock state | Transient by design; re-acquires clock on patch load (matches OU drift philosophy) |
| Continuous (non-snapped) ratios | Anti-pattern; produces non-musical results per research |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| CLK-01 | Phase 7 | Pending |
| CLK-02 | Phase 7 | Pending |
| CLK-03 | Phase 7 | Pending |
| CLK-04 | Phase 7 | Pending |
| CLK-05 | Phase 7 | Pending |
| CLK-06 | Phase 7 | Pending |
| RATE-01 | Phase 8 | Pending |
| RATE-02 | Phase 8 | Pending |
| RATE-03 | Phase 8 | Pending |
| RATE-04 | Phase 9 | Pending |
| RATE-05 | Phase 9 | Pending |
| RATE-06 | Phase 8 | Pending |
| DISP-01 | Phase 10 | Pending |
| DISP-02 | Phase 10 | Pending |
| DISP-03 | Phase 10 | Pending |
| DISP-04 | Phase 9 | Pending |
| DISP-05 | Phase 9 | Pending |
| DISP-06 | Phase 10 | Pending |

**Coverage:**
- v1.1 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0

---
*Requirements defined: 2026-03-07*
*Last updated: 2026-03-07 after roadmap creation*
