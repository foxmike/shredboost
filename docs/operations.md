# ShredBoost — Operations Guide

## Purpose
ShredBoost is an aggressive pre-amp attack shaper, overdrive, and low-end tightener engineered for high-gain thrash rhythm and fast lead guitar. By carving out muddy sub-bass frequencies and boosting critical mid-range frequencies before the nonlinear gain stages, it prevents intermodulation distortion downstream in the high-gain amplifier simulator. The plugin shapes pick transients and clipping characteristics to deliver a defined, articulate note attack that cuts through dense mixes.

## Signal Flow
The internal audio processing chain consists of the following sequential stages:
1. **Input Trim (`input_trim`)**: Adjusts the gain of the incoming signal (-18.0 to +18.0 dB) with linear smoothing.
2. **Pre-Drive Tight HPF (`tight_cut`)**: A 2nd-order RBJ high-pass filter that rolls off flubby low end. In "Precision Tight" voicing mode, it applies a steep cut with a resonant bump (Q = 1.2) at the cutoff frequency; in all other modes, it uses a standard Q = 0.707 response.
3. **Pre-Clip Mid Voicing Bell (`mid_freq`, `mid_amount`)**: A peaking filter that introduces a mid-range emphasis (0 to +9 dB) to voice the signal's drive character.
4. **Transient Pick-Attack Shaper (`pick_attack`)**: Employs two envelope followers (fast $\tau \approx 1\text{ ms}$, slow $\tau \approx 15\text{ ms}$) to identify transients and dynamically scale the pick attack.
5. **Oversampling Upsampler (`oversampling`)**: An FIR polyphase filter that increases the sample rate (1x, 2x, or 4x) to suppress aliasing harmonics.
6. **Nonlinear Clip (`voicing_mode`, `drive`)**: Amplifies the signal by the Drive gain stage and applies one of four clipper topologies:
   - *Yng-Grey*: Smooth arctan waveshaping with an asymmetric even-harmonic term ($\text{atan}(x) + 0.15 \cdot x^2 \cdot e^{-|x|}$).
   - *Precision Tight*: Symmetrical soft-clipping based on a standard hyperbolic tangent ($\text{tanh}(x)$) function.
   - *SD-1*: Asymmetrical soft-clipping emulating unequal silicon diode drops ($\text{tanh}(1.0 \cdot x)$ for positive, $\text{tanh}(1.8 \cdot x)$ for negative cycles).
   - *Clean*: Bypasses the waveshaper entirely for clean boosts.
7. **Oversampling Downsampler**: Decimates the signal back to the host sample rate.
8. **DC Blocker**: A 1st-order high-pass filter ($R = 0.9975$) that centers the waveform and prevents DC offset accumulation from asymmetric clipping.
9. **Post-Drive Tone LPF (`tone`)**: A 2nd-order low-pass filter (1 to 16 kHz) to smooth out high-frequency clipping fizz before output.
10. **Output Level (`level`)**: Applies output gain (-24 to +12 dB) with ramping.
11. **Auto Gain Bypass (`auto_gain`)**: When active, dynamically measures input and output RMS levels and compensates the output volume so that bypassing the plugin maintains consistent perceived loudness.

---

## Controls Reference

All parameters are located on a single main interface panel:

### Input Trim
- **Parameter ID:** `input_trim`
- **Type:** float
- **Range:** -18.0 dB to +18.0 dB, default 0.0 dB
- **Unit:** dB
- **What it does:** Sets the pre-processing input gain before the tight filter and mid-voicing stage.

### Tight Cut
- **Parameter ID:** `tight_cut`
- **Type:** float
- **Range:** 80.0 Hz to 400.0 Hz, default 180.0 Hz (skewed center 180.0 Hz)
- **Unit:** Hz
- **What it does:** Adjusts the cutoff frequency of the pre-drive high-pass filter to control low-end tightening.

### Mid Frequency
- **Parameter ID:** `mid_freq`
- **Type:** float
- **Range:** 500.0 Hz to 1200.0 Hz, default 720.0 Hz (skewed center 720.0 Hz)
- **Unit:** Hz
- **What it does:** Sets the center frequency of the pre-clip mid-range boost bell filter.

### Mid Amount
- **Parameter ID:** `mid_amount`
- **Type:** float
- **Range:** 0.0 dB to 9.0 dB, default 4.0 dB
- **Unit:** dB
- **What it does:** Adjusts the boost intensity of the pre-clip mid-range bell filter.

### Drive
- **Parameter ID:** `drive`
- **Type:** float
- **Range:** 0.0 dB to 36.0 dB, default 12.0 dB
- **Unit:** dB
- **What it does:** Adjusts the pre-saturation gain applied to the signal before entering the clipping engine.

### Tone
- **Parameter ID:** `tone`
- **Type:** float
- **Range:** 1.0 kHz to 16.0 kHz, default 6.5 kHz (skewed center 6.5 kHz)
- **Unit:** kHz
- **What it does:** Sets the cutoff frequency of the post-drive 2nd-order low-pass filter to roll off clip fizz.

### Level
- **Parameter ID:** `level`
- **Type:** float
- **Range:** -24.0 dB to +12.0 dB, default 0.0 dB
- **Unit:** dB
- **What it does:** Adjusts the final output volume level of the processed path.

### Pick Attack
- **Parameter ID:** `pick_attack`
- **Type:** float
- **Range:** 0.0 dB to 9.0 dB, default 2.0 dB
- **Unit:** dB
- **What it does:** Controls the transient pick-attack emphasis factor in the transient shaper stage.

### Voicing Mode
- **Parameter ID:** `voicing_mode`
- **Type:** choice
- **Range:** Yng-Grey (0), Precision Tight (1), SD-1 (2), Clean (3), default Yng-Grey (0)
- **Unit:** none
- **What it does:** Selects the active clipping topology and tight filter response. Smoothly crossfades between modes over 15 ms.

### Oversampling
- **Parameter ID:** `oversampling`
- **Type:** choice
- **Range:** 1x (0), 2x (1), 4x (2), default 2x (1)
- **Unit:** none
- **What it does:** Chooses the oversampling factor (1x, 2x, or 4x) for the clipping engine.

### Auto Gain
- **Parameter ID:** `auto_gain`
- **Type:** bool
- **Range:** Off (0.0) to On (1.0), default Off (0.0)
- **Unit:** none
- **What it does:** Enables or disables automatic RMS-matching gain compensation to keep bypassed and wet levels balanced.

---

## Metering and Visualization
- **Level Meters (`LevelMeters`)**: Polled at 60 Hz. Renders Left and Right peak and RMS channels for input and output (-60 dB to +6 dB), alongside a central Peak Saturation reduction readout showing the instantaneous compression/clipping depth in dB. RMS metering is exponentially smoothed, peak hold decay decays at ~0.222 dB/frame after a 0.5s hold, and saturation reduction decays at 0.25 dB/frame after a 0.25s hold.
- **Spectrum Display (`SpectrumOverlay`)**: Displays a real-time overlay comparing the pre-drive input spectrum (green) and post-drive output spectrum (orange).
- **Transfer Curve Scope (`TransferCurveScope`)**: Renders a real-time X-Y input vs output transfer function plot corresponding to the selected clipping voicing and drive settings.

## Preset System
Four factory presets are compiled directly into the binary via `FactoryPresets.xml`:
1. `Thrash Chug`: Steep low-cut at 280 Hz, mid bump at 620 Hz, high drive, and maximum pick attack.
2. `Yngwie Lead`: Symmetric smooth clipping, 150 Hz tight cut, and +3 dB output volume.
3. `SD-1 Rhythm`: SD-1 asymmetric clipping with a +6 dB mid-hump.
4. `Clean Tight Attack`: Clipping bypassed, emphasizing transient pick shaping (+7 dB) and 220 Hz low-cut.
Save and load behavior is managed by the host DAW using the APVTS standard XML serialization (`getStateInformation`/`setStateInformation`).

## Latency
- Oversampling filter stages introduce latency that varies with selection.
- Latency is 0 samples at 1x oversampling. At 2x and 4x oversampling, the polyphase FIR filter's group delay is measured, stored, and reported dynamically to the host DAW using `setLatencySamples()`.

## Supported Formats
- VST3, LV2, Standalone (specified in `CMakeLists.txt`).

## Known Limitations
- **Transient Compression Downstream**: Because high-gain amplifiers act as limiters, the Pick Attack transient booster is most effective in Clean mode or ahead of low-to-moderate amp gain; its effect is compressed out under extreme downstream high-gain saturation.
- **Oversampling Selection Mute**: Modifying the oversampling factor causes a brief 10 ms mute-fade to allow the DSP to re-initialize safely without clicks.
