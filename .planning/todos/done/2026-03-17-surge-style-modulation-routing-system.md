---
title: Surge-style modulation routing system
created: 2026-03-17
source: Phase 17 panel redesign discussion
priority: medium
---

Replace dedicated per-parameter CV jacks and attenuverter trimpots with a Surge XT-style modulation routing system:

- 2 generic MOD input jacks (assignable to any parameter via right-click menu)
- Per-assignment depth control in menu (replaces physical trimpots)
- Eliminates 5 dedicated CV jacks (Morph, Character, Drift, FM, Phase Offset) and 5 attenuverter trimpots
- Dramatically simplifies panel — from 18 components down to ~10-12

**Why:** Current 12HP panel can't cleanly fit all v1.2 components (FM, Phase Offset CV, attenuverters). The bottom section is overcrowded with labels occluded. Modulation routing solves density while adding flexibility.

**Scope:** This is a significant architecture change — new modulation routing data structures, assignment UI, serialization, and process() rework. Likely its own milestone phase, not a panel tweak. Will break backward compatibility with existing CV jack patches.

**Reference:** Surge XT VCV Rack modules for interaction pattern.

---
**Resolved 2026-06-13 — WON'T DO (out of scope).** Abandoned per REQUIREMENTS.md "Out of Scope": over-engineered for an LFO; direct CV jacks are sufficient. Was the basis for skipping Phase 17. The Forge Noir redesign (Phase 19/20.1, 18HP) solved the panel-density problem that originally motivated this. Closed at v1.3 milestone archive.
