# Phase 1: Plugin Scaffold and Panel - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Buildable VCV Rack 2 plugin with branded SVG panel and module registration. Users can install and load a visible, branded LFO module in VCV Rack that builds cross-platform. The panel SVG establishes the spatial layout that all future phases build controls into.

</domain>

<decisions>
## Implementation Decisions

### Panel dimensions
- 12 HP width (~60mm) — spacious premium feel
- Enough room for generous display and comfortable knob spacing

### Control layout (diamond / 2-1-2 hierarchy)
- **Top third:** Waveform display — generous, approximately one-third of panel height. This is the showcase feature.
- **Below display:** Morph knob — large, hero position, centered. The primary interaction point.
- **Below morph:** Character and Drift knobs — medium-sized, flanking pair left and right.
- **Below character/drift:** Rate knob and Octave pitch control — smaller, utility row.
- **Bottom edge:** Single row of 4 jacks — morph CV in, drift CV in, output, inverted output. All in one line.

### Visual hierarchy
- Display dominates the upper panel — it's what draws the eye
- Morph knob is the largest control, signaling it's the primary knob
- Character and drift are secondary, equal in size to each other
- Rate and pitch are tertiary utility controls
- Jacks are minimal, tucked at the very bottom

### Claude's Discretion
- Brand treatment and aesthetics (logo, typography, visual style within the navy/amber/lavender palette)
- Module browser metadata (name, tags, description)
- Exact knob sizes and spacing within the hierarchy
- SVG structure and coordinate system
- How placeholder positions for future controls are indicated

</decisions>

<specifics>
## Specific Ideas

- The waveform display should feel like the centrepiece — generous space, not an afterthought
- Diamond layout creates a natural visual flow: display -> morph -> character/drift -> rate/pitch -> jacks
- Single jack row keeps the bottom clean and maximizes space for the controls above

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-plugin-scaffold-and-panel*
*Context gathered: 2026-02-25*
