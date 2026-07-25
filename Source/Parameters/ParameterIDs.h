#pragma once

// =============================================================================
// ShredBoost - Frozen Parameter ID Taxonomy
// =============================================================================
// This header declares the permanent, stable string ID constants for every
// automatable parameter in ShredBoost. These IDs are FROZEN: they must never
// change name, value, or meaning once shipped, because they are persisted in
// user-saved presets and DAW project/session state.
//
// Schema versioning: when the parameter set is extended or the semantics of an
// existing parameter change in an incompatible way, increment
// kParameterSchemaVersion and provide a migration path. Existing stable IDs
// must remain valid across schema versions; only their ranges/defaults may be
// reinterpreted under an explicit migration.
//
// This header is intentionally free of any APVTS (AudioProcessorValueTreeState)
// logic, DSP includes, or runtime state. It only declares compile-time
// constants. Internal DSP states (crossfade, mute-fade, RMS tracker, etc.) are
// NOT exposed as parameter IDs and must never be added here.
// =============================================================================

namespace ShredBoost::ParamID
{

// -----------------------------------------------------------------------------
// Schema version
// -----------------------------------------------------------------------------
// Bumped on any incompatible change to the parameter taxonomy. Version 1 is
// the initial shipped schema.
constexpr int kParameterSchemaVersion = 1;

// -----------------------------------------------------------------------------
// Stable parameter ID constants
// -----------------------------------------------------------------------------
// Each constant below documents its human-readable range and default value as
// used by the parameter layout. The string values are the canonical, persisted
// identifiers and must remain byte-for-byte stable.

// input_trim : input trim gain in dB.
//   range  : -18 .. +18 dB
//   default: 0 dB
constexpr const char* input_trim = "input_trim";

// tight_cut : tight (low) cut frequency in Hz.
//   range  : 80 .. 400 Hz
//   default: 180 Hz
constexpr const char* tight_cut = "tight_cut";

// mid_freq : mid-range boost center frequency in Hz.
//   range  : 500 .. 1200 Hz
//   default: 720 Hz
constexpr const char* mid_freq = "mid_freq";

// mid_amount : mid-range boost amount in dB.
//   range  : 0 .. +9 dB
//   default: +4 dB
constexpr const char* mid_amount = "mid_amount";

// drive : drive (preamp gain) amount in dB.
//   range  : 0 .. +36 dB
//   default: +12 dB
constexpr const char* drive = "drive";

// tone : tone (high) shelf/cut frequency in kHz.
//   range  : 1 .. 16 kHz
//   default: 6.5 kHz
constexpr const char* tone = "tone";

// level : output level in dB.
//   range  : -24 .. +12 dB
//   default: 0 dB
constexpr const char* level = "level";

// pick_attack : pick attack emphasis amount in dB.
//   range  : 0 .. +9 dB
//   default: +2 dB
constexpr const char* pick_attack = "pick_attack";

// voicing_mode : voicing mode choice (discrete).
//   range  : 0 .. 3 (choice)
//   default: Yng-Grey
constexpr const char* voicing_mode = "voicing_mode";

// oversampling : oversampling factor choice (discrete).
//   range  : 0 .. 2 (choice)
//   default: 2x
constexpr const char* oversampling = "oversampling";

// auto_gain : automatic gain compensation toggle (bool).
//   range  : off / on
//   default: Off
constexpr const char* auto_gain = "auto_gain";

} // namespace ShredBoost::ParamID
