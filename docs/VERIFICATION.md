# Verification record — v1.0.1

The exact release ROMs passed **17 focused mGBA suites: USA 5, Europe 7, Japan 5**.
Fingerprints and per-suite results are in [verification.json](verification.json).

- Native Cave of Flames lava: ON walks without lava action, burning, bounce,
  health loss or forced room warp; OFF restores the hazard, including when
  switched OFF over lava. Framebuffers were visually inspected.
- Controller-only menu opening, No-Clip switching and return to movement.
- Castle walls, Crenel cliffs, sky/dungeon pits and No-Clip OFF reversibility.
- Native roll, Cape jump, swimming, knockback and held-item behavior.
- EU: 48 OFF/ON × speed × direction cases confirm execution of the relocated
  payload, with no old-payload/AutoTest execution or nested actor iterator.
- EU: native actor-list/register/stack invariants at the reported hardware scene.
- All three regions: independent ROM-byte reconstruction, unchanged headers and
  sealed payload, byte-identical fresh public-source builds, and BPS round trips
  from clean originals producing the exact tested ROMs.

## Hardware confirmation and limits

The user confirmed the isolated EU relocation fix on **GBA SP AGS-101 + EZ-Flash
Omega Definitive Edition**. v1.0.1 integrates that relocation and adds the tested
lava-floor wrapper. The combined build has **not** been separately confirmed on
physical hardware; USA/JP hardware and all flashcart combinations are not claimed.

The old payload location returned incorrect data in the hardware diagnostic.
The unchanged payload now resides at `09070000`. No actor-list repair, suppressed
fault handler or diagnostic screen is included. The exact flashcart-side decode
mechanism remains unproven.

The early-progress EU user save did not reach the charged-sword prerequisite in
the broader movement suite, on both the accepted hardware fix and v1.0.1. That
initial failure is retained in private logs and identified in verification.json;
the complete suite passed on its established regional progress fixture. Dedicated
EU lava, toggle, actor-invariant and relocated-payload tests used the user's save.

The **166 suites below belong to v1.0.0**, not 166 reruns of this update. No new
complete Android matrix, subjective audio review or exhaustive cutscene retest
is claimed. Unchanged systems retain prior coverage; the focused reruns are above.

mGBA development build: `c65e8a3d4666b0ea68a01578232452f31b185332`.
Accelerated runs execute all frames with presentation sync disabled. Private ROMs,
EEPROM fixtures, hardware photos and historical diagnostics are not published.

## Historical v1.0.0 verification

The initial release packaged the sealed USA SCENES and EU/JP builds unchanged.
Its original fingerprints remain in the v1.0.0 source/release history.

- 53 USA and 113 EU/JP exact-hash mGBA suites: **166 total**.
- Independently reconstructed ROM bytes and byte-identical fresh-source builds.
- Menu/HUD framebuffer comparisons, native save/reload, settings corruption fallback.
- Complete native intro/ending replays and restoration of all 1,204 save bytes.
- Two regional native new-game starts from empty EEPROMs; all five EU languages.
- Actor catalog/cleanup/freeze/teleport, movement/exploration, flags, collections and cheats.
- BPS files re-applied again while preparing this public package: exact target hashes.
- Tester confirmation: menu opening and No-Clip; not a documented full Android matrix.

mGBA development build: `c65e8a3d4666b0ea68a01578232452f31b185332`.
Accelerated tests execute all emulation frames with presentation sync disabled.
Some credits waiting timers are shortened, not native sequence entries/scripts.
Live audio quality and physical hardware were not separately validated.

Private EEPROM fixtures and historical workspace logs are deliberately not
published. Test source and this sanitized evidence summary are included.
No runtime or patch bytes were changed just for GitHub publication.
