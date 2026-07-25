#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <array>

class TransferCurveScopeTests;

namespace ShredBoost
{

/**
    A live X-Y transfer curve plot y = f(v) with a moving dot representing
    the instantaneous input amplitude, featuring smooth morphing when the voicing
    mode changes.

    This component is a Timer-driven consumer. It reads telemetry lock-free
    without ever calling the audio thread or taking locks.
*/
class TransferCurveScope : public juce::Component,
                           private juce::Timer
{
    friend class ::TransferCurveScopeTests;

public:
    TransferCurveScope() noexcept;
    ~TransferCurveScope() override;

    // --- Telemetry feeding public API ---

    /** Directly pushes a new input sample value to update the moving dot. */
    void pushInputSample (float sample) noexcept;

    /** Sets the current voicing mode to transition the curve. */
    void setVoicingMode (int mode) noexcept;

    /** Alternative API: Bind the scope to external atomic variables for polling.
        The scope's internal timer will read from these pointers if they are set.
    */
    void setTelemetrySources (const std::atomic<float>* amplitudeSource,
                              const std::atomic<int>* voicingModeSource) noexcept;

    // --- Styling options ---
    void setCurveColour (juce::Colour newColour) noexcept;
    void setDotColour (juce::Colour newColour) noexcept;
    void setGridColour (juce::Colour newColour) noexcept;

    // --- Component overrides ---
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    static float applyTopology (float input, int mode) noexcept;

    // Pointers to external atomic telemetry sources (optional)
    const std::atomic<float>* externalAmplitudeSource = nullptr;
    const std::atomic<int>* externalVoicingModeSource = nullptr;

    // Internal telemetry states
    std::atomic<float> instantaneousAmplitude { 0.0f };
    float smoothedAmplitude = 0.0f;

    // Morphing transition states
    std::array<float, 4> currentWeights { 1.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> targetWeights { 1.0f, 0.0f, 0.0f, 0.0f };

    // Visual styles
    juce::Colour curveColour { juce::Colour (0xff00e5ff) }; // neon cyan
    juce::Colour dotColour { juce::Colour (0xffffffff) };   // white
    juce::Colour gridColour { juce::Colour (0xffffffff).withAlpha (0.07f) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransferCurveScope)
};

} // namespace ShredBoost
