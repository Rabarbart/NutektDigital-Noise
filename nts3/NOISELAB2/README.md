# NOISELAB2 — experimental Korg NTS-3 instrument

Status: **EXPERIMENTAL / HOST TESTED**. No Editor or physical NTS-3 verification.

## Goals
- Percussive white/pink/brown noise hits, longer noise shots, hand-played filter sweeps.
- One touch event launches a HIT; holding the pad >80 ms fades into SUSTAIN.
- Releasing after a hold uses the RELEASE envelope. Quick taps remain one-shots.
- Four HIT voice envelopes allow short bursts to overlap.
- Noise types: WHITE, PINK, BROWN, FILTERED, MIX, PULSE, CRACKLE, DIGITAL.
- No autonomous MOTION/RATE; gesture and remappable parameters drive the instrument.

## Eight editable parameters
1. TYPE (0–7), default WHITE, not mapped
2. ATTACK (0–1023), 0.3–150 ms, not mapped
3. DECAY (0–1023), 8 ms–3 s, default mapping **Y**
4. RELEASE (0–1023), 7 ms–2.5 s, not mapped
5. TONE (0–1023), LP → dry → HP, default mapping **X**
6. RESONANCE (0–1023), Q ~0.7–3.5, not mapped
7. TEXTURE (0–1023), spectral grain/impulse density, not mapped
8. LEVEL (0–1023), default mapping **FX DEPTH**

All controls are defined in the NTS-3 header and can be reassigned in the Korg Editor.
Raw touch coordinates are NOT wired to DSP parameters; touch events only gate/trigger voices.

## Build and safety
Copy \`nts3/NOISELAB2\` inside \`logue-sdk/platform/nts-3_kaoss/\`, run:
\`\`\`sh
make GCC_BIN_PATH=/usr/bin
make install GCC_BIN_PATH=/usr/bin
\`\`\`
This uses official Korg SDK build rules and target. The local \`sdk_common/_unit_base.c\`
suppresses the unused weak fallback header; check that \`.unit_header\` is exactly
376 bytes (not 752). Validate ELF32, ARM EABI5, hard float, target 0x0607,
API 0x00020000, 8 descriptors and remap defaults.

Developer ID BART (0x42415254) is the user's technical ID, not verified as a
publicly registered Korg ID. Unit ID NL20 (0x4E4C3230) is new for this prototype.
The existing NOISELAB 1.0 sources and binaries remain unchanged.

## Known limitations
- Gesture transition is based on 80 ms touch time; very short DECAY can leave a
  perceptible dip before a held sweep. Evaluate on hardware before refining.
- Filter Q and parameter curves are engineering starting points, not listening-approved.
- Single touch input and mono generated content sent equally to left and right.
- Host tests are not an Editor import or physical instrument test.
