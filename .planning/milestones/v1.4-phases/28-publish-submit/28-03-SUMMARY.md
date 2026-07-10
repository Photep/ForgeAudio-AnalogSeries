---
phase: 28-publish-submit
plan: 03
status: complete
completed: 2026-07-10
requirements: [PUB-02]
---

# 28-03 Summary — VCV Library submission

## What was done

Submitted the plugin to the VCV Library by opening EXACTLY ONE issue on the public
`VCVRack/library` repo, titled with the plugin slug and pinning the immutable release commit.

- **Task 1 (confirm + draft):** Final read of `plugin.json` (confirm, not edit) — slug
  `ForgeAudio-AnalogSeries`, module slug `ForgeAnalogLFO`, version `2.0.0`, license
  `GPL-3.0-or-later`, sourceUrl/manualUrl as declared. Re-captured
  `git rev-parse v2.0.0^{commit}` = `4d7b0a81f7aabed83626a11951956fff173b6ad7` and confirmed
  it equals the hash recorded by 28-01. Asserted **zero** existing slug-titled issues on
  `VCVRack/library` (no duplicate). Drafted the issue body with the full 40-char hash.
- **Task 2 (file — human-gated):** Operator approved the drafted title/body and opted to
  omit the optional email. Ran
  `gh issue create --repo VCVRack/library --title "ForgeAudio-AnalogSeries" --body-file <draft>`.
  Verified title == `ForgeAudio-AnalogSeries` and the body contains the full 40-char hash
  (no bare branch/tag name where the ref belongs).
- **Task 3 (record):** Wrote the issue URL and submitted commit hash into STATE.md as the
  permanent VCV Library submission record.

## Key result

- **Submission issue: https://github.com/VCVRack/library/issues/929**
  - Title: `ForgeAudio-AnalogSeries` (plugin slug — VCV tooling keys the thread on this).
  - Build ref: `4d7b0a81f7aabed83626a11951956fff173b6ad7` (immutable full hash, tagged `v2.0.0`).
  - Fields: name, license GPL-3.0-or-later, version 2.0.0, sourceUrl, manualUrl.
- No GitHub Release, no `.vcvplugin` attached — VCV's `rack-plugin-toolchain` clones the
  public sourceUrl at the submitted commit and builds every platform itself.
- PUB-02 satisfied; ROADMAP success criterion 4 (issue URL recorded for future updates) met.

## Notes

- **#929 is the permanent thread.** All future version bumps are comments on this issue —
  never a second submission issue.
- Notification path: operator is auto-subscribed as issue author (GitHub), not via a body
  email (which was deliberately omitted to avoid a permanent public contact address).
