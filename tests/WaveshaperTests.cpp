// =============================================================================
// ShredBoost - Waveshaper Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Verifies the
// Phase D nonlinear waveshaper building blocks:
//
//   * DC offset      : drives a signal through the Asymmetric SD-1 clipper
//                      (NonlinearClipper voicing_mode 2) followed by the
//                      DcBlocker and asserts the output mean is approximately
//                      zero, proving the DC blocker removes the offset the
//                      asymmetric waveshaper introduces.
//   * Click-free     : switches NonlinearClipper voicing_mode during
//     crossfade       processing and asserts the sample-to-sample difference
//                      during the 15 ms topology crossfade window stays within
//                      a bounded threshold (no step discontinuity).
//
// Tests are auto-registered with juce::UnitTest and discovered by the
// juce::UnitTestRunner launched from tests/TestMain.cpp.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Source/DSP/NonlinearClipper.h"
#include "Source/DSP/DcBlocker.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 64;
constexpr double kTwoPi = 6.283185307179586476925286766559;

} // namespace

// =============================================================================
// DC offset removal
// =============================================================================
class WaveshaperDCOffsetTests final : public juce::UnitTest
{
public:
    WaveshaperDCOffsetTests()
        : juce::UnitTest ("Waveshaper DC blocker removes asymmetric offset", "Waveshaper")
    {
    }

    void runTest() override
    {
        beginTest ("Asymmetric SD-1 clipper (mode 2) + DcBlocker yields ~0 mean");

        const double frequency = 220.0;
        const double amplitude = 0.8;
        const int totalSamples = 100000;
        const int measureStart = 20000; // well past the DC-blocker transient

        ShredBoost::NonlinearClipper clipper;
        clipper.prepare (kSampleRate, kBlockSize);
        clipper.reset();
        clipper.setVoicingMode (2); // Asymmetric SD-1 style clipper

        ShredBoost::DcBlocker blocker;
        blocker.prepare (kSampleRate, kBlockSize);
        blocker.reset();

        juce::AudioBuffer<float> clipBuffer (1, kBlockSize);
        juce::AudioBuffer<float> blockBuffer (1, kBlockSize);

        double sumWithBlocker = 0.0;
        double sumWithoutBlocker = 0.0;
        int measured = 0;

        int sample = 0;
        while (sample < totalSamples)
        {
            const int n = std::min (kBlockSize, totalSamples - sample);

            for (int i = 0; i < n; ++i)
            {
                const double s = amplitude
                    * std::sin (kTwoPi * frequency * static_cast<double> (sample + i) / kSampleRate);
                clipBuffer.setSample (0, i, static_cast<float> (s));
            }

            // Clipper processes in place; clipBuffer now holds the asymmetric
            // waveshaper output (the signal that carries a DC offset).
            juce::dsp::AudioBlock<float> clipBlock (clipBuffer);
            clipper.process (clipBlock);

            // Copy the clipped signal into a second buffer for the DC blocker
            // so we can still read the pre-blocker offset from clipBuffer.
            for (int i = 0; i < n; ++i)
                blockBuffer.setSample (0, i, clipBuffer.getSample (0, i));

            juce::dsp::AudioBlock<float> blockBlock (blockBuffer);
            blocker.process (blockBlock);

            if (sample + n >= measureStart)
            {
                const int startIdx = std::max (0, measureStart - sample);
                for (int i = startIdx; i < n; ++i)
                {
                    sumWithBlocker += static_cast<double> (blockBuffer.getSample (0, i));
                    sumWithoutBlocker += static_cast<double> (clipBuffer.getSample (0, i));
                    ++measured;
                }
            }

            sample += n;
        }

        expect (measured > 0, "Measurement window must contain samples");

        const double meanWithBlocker = sumWithBlocker / static_cast<double> (measured);
        const double meanWithoutBlocker = sumWithoutBlocker / static_cast<double> (measured);

        // The asymmetric clipper must introduce a measurable DC offset; this
        // proves the test is exercising the offset path and not a no-op.
        expect (std::abs (meanWithoutBlocker) > 1.0e-3,
                "Asymmetric SD-1 clipper should introduce a DC offset without the blocker; mean="
                    + juce::String (meanWithoutBlocker, 6));

        // The DC blocker must remove that offset down to a tiny residual.
        expect (std::abs (meanWithBlocker) < 5.0e-3,
                "DC blocker should remove the asymmetric offset; mean="
                    + juce::String (meanWithBlocker, 6)
                    + " (pre-blocker mean was " + juce::String (meanWithoutBlocker, 6) + ")");
    }
};

// =============================================================================
// Click-free topology crossfade
// =============================================================================
class WaveshaperCrossfadeTests final : public juce::UnitTest
{
public:
    WaveshaperCrossfadeTests()
        : juce::UnitTest ("Waveshaper click-free topology crossfade", "Waveshaper")
    {
    }

    void runTest() override
    {
        beginTest ("voicing_mode switch keeps sample-to-sample diff bounded during 15 ms crossfade");

        const double frequency = 330.0;
        const double amplitude = 0.7;
        const int crossfadeSamples =
            static_cast<int> (std::round (kSampleRate * 0.015)); // 720 @ 48 kHz
        const int preWindow = crossfadeSamples;
        const int postWindow = crossfadeSamples + 200;
        const int totalSamples = preWindow + postWindow + 1000;

        const int startMode = 1;  // symmetric tanh
        const int targetMode = 3; // linear passthrough - very different gain

        ShredBoost::NonlinearClipper clipper;
        clipper.prepare (kSampleRate, 1);
        clipper.reset();
        clipper.setVoicingMode (startMode);

        std::vector<float> output (static_cast<std::size_t> (totalSamples), 0.0f);
        juce::AudioBuffer<float> buffer (1, 1);

        for (int n = 0; n < totalSamples; ++n)
        {
            const double s = amplitude
                * std::sin (kTwoPi * frequency * static_cast<double> (n) / kSampleRate);
            buffer.setSample (0, 0, static_cast<float> (s));

            juce::dsp::AudioBlock<float> block (buffer);
            clipper.process (block);

            output[static_cast<std::size_t> (n)] = buffer.getSample (0, 0);

            // Flip the voicing immediately after the pre-crossfade window so
            // the very next process() call begins the equal-power crossfade.
            if (n == preWindow)
                clipper.setVoicingMode (targetMode);
        }

        // Steady-state sample-to-sample difference just before the switch.
        double preMaxDiff = 0.0;
        for (int n = preWindow - crossfadeSamples + 1; n <= preWindow; ++n)
        {
            const double diff =
                std::abs (static_cast<double> (output[static_cast<std::size_t> (n)])
                          - static_cast<double> (output[static_cast<std::size_t> (n - 1)]));
            preMaxDiff = std::max (preMaxDiff, diff);
        }

        // Worst-case sample-to-sample difference across the 15 ms crossfade.
        double crossfadeMaxDiff = 0.0;
        for (int n = preWindow + 1; n <= preWindow + crossfadeSamples; ++n)
        {
            const double diff =
                std::abs (static_cast<double> (output[static_cast<std::size_t> (n)])
                          - static_cast<double> (output[static_cast<std::size_t> (n - 1)]));
            crossfadeMaxDiff = std::max (crossfadeMaxDiff, diff);
        }

        // Sanity: the voicing must actually have changed, otherwise the
        // click-free claim is vacuous. Compare steady-state RMS well away
        // from the crossfade region.
        const int postSteadyStart = preWindow + crossfadeSamples + 200;
        const int postSteadyEnd = std::min (totalSamples, postSteadyStart + crossfadeSamples);
        const int preSteadyStart = preWindow - crossfadeSamples;

        double preSumSq = 0.0;
        double postSumSq = 0.0;
        int preCount = 0;
        int postCount = 0;
        for (int n = preSteadyStart; n < preWindow; ++n, ++preCount)
            preSumSq += static_cast<double> (output[static_cast<std::size_t> (n)])
                * static_cast<double> (output[static_cast<std::size_t> (n)]);
        for (int n = postSteadyStart; n < postSteadyEnd; ++n, ++postCount)
            postSumSq += static_cast<double> (output[static_cast<std::size_t> (n)])
                * static_cast<double> (output[static_cast<std::size_t> (n)]);

        const double preRms = preCount > 0 ? std::sqrt (preSumSq / preCount) : 0.0;
        const double postRms = postCount > 0 ? std::sqrt (postSumSq / postCount) : 0.0;

        expect (std::abs (postRms - preRms) > 1.0e-3,
                "voicing_mode switch should change the steady-state output level (pre RMS="
                    + juce::String (preRms, 5) + ", post RMS=" + juce::String (postRms, 5) + ")");

        // Absolute click bound: a step discontinuity would produce a diff on
        // the order of the signal amplitude (~0.5+); a smooth equal-power
        // crossfade stays well below this.
        const double absoluteClickBound = 0.15;
        expect (crossfadeMaxDiff < absoluteClickBound,
                "Crossfade must be click-free (absolute bound); max sample diff="
                    + juce::String (crossfadeMaxDiff, 6));

        // Relative bound: the crossfade diff must stay comparable to the
        // steady-state diff (no step). A 2x margin absorbs the equal-power
        // blending term while still rejecting discontinuities.
        expect (crossfadeMaxDiff <= preMaxDiff * 2.0 + 1.0e-6,
                "Crossfade diff must be bounded relative to steady state; pre max diff="
                    + juce::String (preMaxDiff, 6)
                    + ", crossfade max diff=" + juce::String (crossfadeMaxDiff, 6));
    }
};

// Static registrations make these tests discoverable by juce::UnitTestRunner.
static WaveshaperDCOffsetTests sWaveshaperDCOffsetTests;
static WaveshaperCrossfadeTests sWaveshaperCrossfadeTests;
