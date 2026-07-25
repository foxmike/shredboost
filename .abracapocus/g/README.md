# g

## Goal

Native JUCE vector GUI implementation in Source/UI/. Custom LookAndFeel and dark industrial rotaries (ShredKnob). Source/UI/TransferCurveScope.h/.cpp: live X-Y transfer curve plot y=f(v) with moving dot indicating instantaneous input amplitude and morphing crossfade during voicing changes. Source/UI/SpectrumOverlay.h/.cpp: 512-bin Welch-averaged pre/post FFT spectrum display. Source/UI/LevelMeters.h/.cpp: Peak + RMS I/O meters and Drive Saturation readout (peak reduction). PluginEditor connects all components with APVTS attachments, gain-matched bypass button, and resizable window.

## Notes

- Async: Wave 1: g.1 (ShredLookAndFeel), g.2 (TransferCurveScope), g.3 (SpectrumOverlay), and g.4 (LevelMeters) run in parallel. Final wave: g.5 connects UI components into PluginEditor and updates CMakeLists.txt (depends on g.1, g.2, g.3, g.4).
- Risk: GUI thread must only read atomic telemetry / FIFO queues, never lock or block the audio thread.
