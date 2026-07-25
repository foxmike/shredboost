// =============================================================================
// ShredBoost - DSP Filter Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Verifies the
// Phase C DSP building blocks by measuring *actual* filter/stage output in
// response to sine probes and asserting against analytic RBJ / dB-gain
// expected values:
//
//   * TightFilter  : second-order high-pass. Attenuation below tight_cut,
//                    unity above, and the Q=1.2 "Precision Tight" resonant
//                    overshoot at cutoff (~+1.8 dB) while Q=0.707 (Butterworth)
//                    shows no overshoot (~-3 dB at cutoff).
//   * MidVoicingFilter : RBJ peaking-EQ mid bell. Gain boost peaks at mid_freq
//                    with the fixed Q_m=0.9, up to +9 dB.
//   * InputTrimStage   : output gain matches Gin = 10^(trim/20) within
//                    tolerance for representative trim values.
//
// Tests are auto-registered with juce::UnitTest and discovered by the
// juce::UnitTestRunner launched from tests/TestMain.cpp.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "Source/DSP/TightFilter.h"
#include "Source/DSP/MidVoicingFilter.h"
#include "Source/DSP/InputTrimStage.h"

#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;
// The probe must outlive the 20 ms parameter smoothers (≈ 960 samples at 48 kHz)
// and the biquad transient. Measure the steady-state amplitude of the tail.
constexpr int kProbeTotalSamples = 16384;
constexpr int kProbeMeasureStart = 8192;
constexpr double kTwoPi = 6.283185307179586476925286766559;

// Feeds a unit-amplitude sine at freqHz through the stage and returns the
// measured steady-state output magnitude in dB (20*log10(peakAmplitude)).
// Works for any stage exposing `float process(float)`.
template <typename Stage>
double measureSteadyStateGainDb (Stage& stage, double freqHz)
{
    double peakAmplitude = 0.0;

    for (int i = 0; i < kProbeTotalSamples; ++i)
    {
        const double inputSample = std::sin (kTwoPi * freqHz * static_cast<double> (i) / kSampleRate);
        const double outputSample = static_cast<double> (stage.process (static_cast<float> (inputSample)));

        if (i >= kProbeMeasureStart)
        {
            const double absoluteValue = std::abs (outputSample);
            if (absoluteValue > peakAmplitude)
                peakAmplitude = absoluteValue;
        }
    }

    if (peakAmplitude <= 1.0e-12)
        return -300.0;

    return 20.0 * std::log10 (peakAmplitude);
}

} // namespace

// =============================================================================
// TightFilter
// =============================================================================
class TightFilterTests final : public juce::UnitTest
{
public:
    TightFilterTests() : juce::UnitTest ("TightFilter response", "FilterTests") {}

    void runTest() override
    {
        testHighPassAttenuationBelowCutoff();
        testHighPassUnityAboveCutoff();
        testPrecisionTightResonantOvershootVersusButterworth();
    }

private:
    void testHighPassAttenuationBelowCutoff()
    {
        beginTest ("TightFilter attenuates below tight_cut (Butterworth)");

        ShredBoost::TightFilter filter;
        filter.prepare (kSampleRate);
        filter.reset();
        filter.setResonantMode (false);
        filter.setCutoff (200.0f);

        // 0.5x cutoff sits well into the stopband; RBJ HPF gives ~-12.3 dB.
        const double measuredDb = measureSteadyStateGainDb (filter, 100.0);

        expect (measuredDb < -6.0,
                "Expected strong attenuation below tight_cut, got " + juce::String (measuredDb, 4) + " dB");
        expectWithinAbsoluteError (measuredDb, -12.3052, 0.25,
                                  "Below-cutoff gain should match RBJ analytic value (~-12.3 dB)");
    }

    void testHighPassUnityAboveCutoff()
    {
        beginTest ("TightFilter is ~unity above tight_cut (Butterworth)");

        ShredBoost::TightFilter filter;
        filter.prepare (kSampleRate);
        filter.reset();
        filter.setResonantMode (false);
        filter.setCutoff (200.0f);

        // 4x cutoff is well into the passband; RBJ HPF gives ~-0.017 dB.
        const double measuredDb = measureSteadyStateGainDb (filter, 800.0);

        expectWithinAbsoluteError (measuredDb, 0.0, 0.1,
                                  "Above-cutoff gain should be unity (0 dB), got "
                                      + juce::String (measuredDb, 4) + " dB");
    }

    void testPrecisionTightResonantOvershootVersusButterworth()
    {
        beginTest ("Precision Tight (Q=1.2) shows ~+1.8 dB overshoot at cutoff; Butterworth (Q=0.707) does not");

        const double tightCut = 200.0;
        const double precisionTightExpectedDb = 20.0 * std::log10 (1.2); // ≈ +1.58 dB (≈ +1.8 dB)

        // Precision Tight mode: Q = 1.2 -> |H(w0)| = Q -> resonant overshoot.
        ShredBoost::TightFilter precisionTight;
        precisionTight.prepare (kSampleRate);
        precisionTight.reset();
        precisionTight.setResonantMode (true);
        precisionTight.setCutoff (static_cast<float> (tightCut));

        const double measuredPrecisionDb = measureSteadyStateGainDb (precisionTight, tightCut);

        expect (measuredPrecisionDb > 1.0,
                "Q=1.2 should show a positive resonant overshoot at cutoff, got "
                    + juce::String (measuredPrecisionDb, 4) + " dB");
        expectWithinAbsoluteError (measuredPrecisionDb, precisionTightExpectedDb, 0.25,
                                  "Q=1.2 overshoot at cutoff should match the analytic Q gain "
                                  "(~" + juce::String (precisionTightExpectedDb, 2)
                                  + " dB, i.e. the documented ~+1.8 dB Precision Tight overshoot)");

        // Butterworth mode: Q = 1/sqrt(2) -> |H(w0)| = Q -> ~-3.01 dB, no overshoot.
        ShredBoost::TightFilter butterworth;
        butterworth.prepare (kSampleRate);
        butterworth.reset();
        butterworth.setResonantMode (false);
        butterworth.setCutoff (static_cast<float> (tightCut));

        const double measuredButterworthDb = measureSteadyStateGainDb (butterworth, tightCut);

        expect (measuredButterworthDb < 0.0,
                "Q=0.707 should NOT overshoot at cutoff, got "
                    + juce::String (measuredButterworthDb, 4) + " dB");
        expectWithinAbsoluteError (measuredButterworthDb, -3.0103, 0.2,
                                  "Q=0.707 cutoff gain should match the analytic Butterworth value (~-3 dB)");

        // The Precision Tight response must be measurably higher than Butterworth.
        expect (measuredPrecisionDb - measuredButterworthDb > 3.0,
                "Precision Tight should exceed Butterworth at cutoff by ~4.6 dB");
    }
};

// =============================================================================
// MidVoicingFilter
// =============================================================================
class MidVoicingFilterTests final : public juce::UnitTest
{
public:
    MidVoicingFilterTests() : juce::UnitTest ("MidVoicingFilter mid bell", "FilterTests") {}

    void runTest() override
    {
        testMidBellPeaksAtMidFreqUpToPlus9dB();
        testMidBellAmountSweepMatchesAnalyticGain();
        testMidBellShapeFallsOffAwayFromMidFreq();
    }

private:
    void testMidBellPeaksAtMidFreqUpToPlus9dB()
    {
        beginTest ("MidVoicingFilter peaks near mid_freq at +9 dB for max amount");

        const double midFreq = 720.0;
        const double amountDb = 9.0;

        ShredBoost::MidVoicingFilter filter;
        filter.prepare (kSampleRate);
        filter.reset();
        filter.setFrequency (static_cast<float> (midFreq));
        filter.setAmount (static_cast<float> (amountDb));

        const double measuredDb = measureSteadyStateGainDb (filter, midFreq);

        expectWithinAbsoluteError (measuredDb, amountDb, 0.15,
                                  "Peak gain at mid_freq should equal the amount (+9 dB), got "
                                      + juce::String (measuredDb, 4) + " dB");
    }

    void testMidBellAmountSweepMatchesAnalyticGain()
    {
        beginTest ("MidVoicingFilter peak gain tracks setAmount across 0..+9 dB with fixed Q_m=0.9");

        const double midFreq = 720.0;
        const double amountsDb[] = { 0.0, 3.0, 6.0, 9.0 };

        for (const double amountDb : amountsDb)
        {
            ShredBoost::MidVoicingFilter filter;
            filter.prepare (kSampleRate);
            filter.reset();
            filter.setFrequency (static_cast<float> (midFreq));
            filter.setAmount (static_cast<float> (amountDb));

            const double measuredDb = measureSteadyStateGainDb (filter, midFreq);

            expectWithinAbsoluteError (measuredDb, amountDb, 0.1,
                                      "Peak gain at mid_freq should match amount="
                                          + juce::String (amountDb, 1) + " dB, got "
                                          + juce::String (measuredDb, 4) + " dB");
        }
    }

    void testMidBellShapeFallsOffAwayFromMidFreq()
    {
        beginTest ("MidVoicingFilter mid-bell falls off away from mid_freq (peaking curve)");

        const double midFreq = 720.0;
        const double amountDb = 9.0;

        ShredBoost::MidVoicingFilter filter;
        filter.prepare (kSampleRate);
        filter.reset();
        filter.setFrequency (static_cast<float> (midFreq));
        filter.setAmount (static_cast<float> (amountDb));

        const double atMidDb = measureSteadyStateGainDb (filter, midFreq);
        const double belowMidDb = measureSteadyStateGainDb (filter, midFreq * 0.5);
        const double aboveMidDb = measureSteadyStateGainDb (filter, midFreq * 2.0);

        expect (atMidDb > belowMidDb,
                "Bell should peak at mid_freq, not below it (at=" + juce::String (atMidDb, 3)
                    + " below=" + juce::String (belowMidDb, 3) + ")");
        expect (atMidDb > aboveMidDb,
                "Bell should peak at mid_freq, not above it (at=" + juce::String (atMidDb, 3)
                    + " above=" + juce::String (aboveMidDb, 3) + ")");
        // With +9 dB amount and Q_m=0.9, the 0.5x/2x offsets sit on the bell
        // skirt (analytically ~+2.29 dB / ~+1.70 dB) - well below the +9 dB peak.
        expect (belowMidDb < atMidDb - 3.0,
                "Bell skirt should be substantially below the peak");
        expect (aboveMidDb < atMidDb - 3.0,
                "Bell skirt should be substantially below the peak");
    }
};

// =============================================================================
// InputTrimStage
// =============================================================================
class InputTrimStageTests final : public juce::UnitTest
{
public:
    InputTrimStageTests() : juce::UnitTest ("InputTrimStage gain", "FilterTests") {}

    void runTest() override
    {
        testTrimGainMatchesDecibelToLinear();
        testUnityAtZeroTrim();
    }

private:
    void testTrimGainMatchesDecibelToLinear()
    {
        beginTest ("InputTrimStage output gain matches Gin = 10^(trim/20)");

        const float trimValuesDb[] = { -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f };

        for (const float trimDb : trimValuesDb)
        {
            ShredBoost::InputTrimStage stage;
            stage.prepare (kSampleRate);
            stage.reset();
            stage.setTrimDb (trimDb);

            // The stage gain is flat with frequency; probe at a representative
            // audible frequency. The 20 ms linear dB smoother has fully
            // settled by the measurement window.
            const double measuredDb = measureSteadyStateGainDb (stage, 1000.0);
            const double expectedDb = static_cast<double> (trimDb);

            expectWithinAbsoluteError (measuredDb, expectedDb, 0.02,
                                      "Trim " + juce::String (trimDb, 2)
                                          + " dB should yield gain Gin=10^(trim/20), measured="
                                          + juce::String (measuredDb, 4) + " dB");
        }
    }

    void testUnityAtZeroTrim()
    {
        beginTest ("InputTrimStage is unity (0 dB) at trim = 0 dB");

        ShredBoost::InputTrimStage stage;
        stage.prepare (kSampleRate);
        stage.reset();
        stage.setTrimDb (0.0f);

        const double measuredDb = measureSteadyStateGainDb (stage, 1000.0);

        expectWithinAbsoluteError (measuredDb, 0.0, 0.01,
                                  "Zero trim should produce unity gain, got "
                                      + juce::String (measuredDb, 4) + " dB");
    }
};

// Static registrations make these tests discoverable by juce::UnitTestRunner.
static TightFilterTests sTightFilterTests;
static MidVoicingFilterTests sMidVoicingFilterTests;
static InputTrimStageTests sInputTrimStageTests;
