#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class LevelMetersTests;

namespace ShredBoost
{

/**
    A JUCE Component that displays Peak + RMS input/output level meters
    and a Drive Saturation (peak reduction in dB) readout.

    This component is a Timer-driven consumer. It polls atomic telemetry handles
    from the audio processor at 60 Hz on the message thread. It never locks,
    blocks, or makes audio thread calls.
*/
class LevelMeters : public juce::Component,
                    private juce::Timer
{
    friend class ::LevelMetersTests;

public:
    LevelMeters() noexcept;
    ~LevelMeters() override;

    /** Binds the meter to external atomic telemetry handles. */
    void setTelemetrySources (const std::atomic<float>* inputPeakLeft,
                              const std::atomic<float>* inputPeakRight,
                              const std::atomic<float>* inputRmsLeft,
                              const std::atomic<float>* inputRmsRight,
                              const std::atomic<float>* outputPeakLeft,
                              const std::atomic<float>* outputPeakRight,
                              const std::atomic<float>* outputRmsLeft,
                              const std::atomic<float>* outputRmsRight,
                              const std::atomic<float>* peakReductionDb) noexcept;

    // Component overrides
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    // Layout / Drawing helpers
    void drawMeterGroup (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                         float rmsDbL, float peakDbL,
                         float rmsDbR, float peakDbR,
                         const juce::String& groupLabel);

    void drawVerticalMeter (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                            float rmsDb, float peakDb, bool isLeft, const juce::String& channelLabel);

    void drawSaturationGroup (juce::Graphics& g, const juce::Rectangle<float>& bounds, float reductionDb);

    // External atomic telemetry handles
    const std::atomic<float>* externalInputPeakLeft = nullptr;
    const std::atomic<float>* externalInputPeakRight = nullptr;
    const std::atomic<float>* externalInputRmsLeft = nullptr;
    const std::atomic<float>* externalInputRmsRight = nullptr;
    const std::atomic<float>* externalOutputPeakLeft = nullptr;
    const std::atomic<float>* externalOutputPeakRight = nullptr;
    const std::atomic<float>* externalOutputRmsLeft = nullptr;
    const std::atomic<float>* externalOutputRmsRight = nullptr;
    const std::atomic<float>* externalPeakReductionDb = nullptr;

    // Displayed values (smoothed / decayed in dB domain)
    float displayedInputPeakLeft = -100.0f;
    float displayedInputPeakRight = -100.0f;
    float displayedInputRmsLeft = -100.0f;
    float displayedInputRmsRight = -100.0f;

    float displayedOutputPeakLeft = -100.0f;
    float displayedOutputPeakRight = -100.0f;
    float displayedOutputRmsLeft = -100.0f;
    float displayedOutputRmsRight = -100.0f;

    float displayedReductionDb = 0.0f;

    // Peak hold counters (in frames)
    int inputPeakHoldLeft = 0;
    int inputPeakHoldRight = 0;
    int outputPeakHoldLeft = 0;
    int outputPeakHoldRight = 0;
    int reductionHold = 0;

    // Ballistics constants
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 6.0f;
    static constexpr float peakDecayRate = 20.0f / (1.5f * 60.0f); // 20 dB over 1.5 seconds at 60 Hz -> ~0.222 dB/frame
    static constexpr float reductionDecayRate = 15.0f / (1.0f * 60.0f); // 15 dB over 1.0 seconds at 60 Hz -> 0.25 dB/frame
    static constexpr float rmsSmoothFactor = 0.2f; // simple exponential smoothing factor for RMS

    static constexpr int peakHoldFrames = 30; // 0.5s hold at 60 Hz
    static constexpr int reductionHoldFrames = 15; // 0.25s hold at 60 Hz

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeters)
};

} // namespace ShredBoost
