# Verification record

This release packages the sealed USA SCENES and EU/JP regional builds unchanged.
The patch and output fingerprints are machine-readable in [verification.json](verification.json).

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
