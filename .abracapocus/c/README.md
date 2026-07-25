# c

## Goal

Implement pre-drive filtering and calibration stages. Source/DSP/InputTrimStage.h/.cpp: smoothed gain Gin = 10^(trim/20) with peak/RMS signal level metering. Source/DSP/TightFilter.h/.cpp: 2nd-order RBJ HPF with tight_cut frequency (80-400 Hz); Q=0.707 standard Butterworth, Q=1.2 resonant bump (+1.8 dB overshoot at cutoff) in Precision Tight mode. Source/DSP/MidVoicingFilter.h/.cpp: 2nd-order RBJ peaking bell filter centered at mid_freq (500-1200 Hz), boosting up to +9 dB before non-linear stage with Q_m=0.9. tests/FilterTests.cpp verifies frequency response, Q-factor resonance bump, mid-bell peaking curve, and input trim attenuation.

## Notes

- Async: Wave 1: c.1 (TightFilter), c.2 (MidVoicingFilter), and c.3 (InputTrimStage) run in parallel on disjoint files. Final wave: c.4 creates tests/FilterTests.cpp and updates CMakeLists.txt (depends on c.1, c.2, c.3).
- Risk: Filter coefficient calculation must handle sample rate changes cleanly and flush denormals.
