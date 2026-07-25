// =============================================================================
// ShredBoost - Oversampling / Bypass Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Verifies the
// Phase E behavioural claims of the oversampling + auto-gain bypass path:
//
//   * OversamplingEngine alias rejection : drives a high-frequency sine
//     through upsample -> hard-clip nonlinearity -> downsample and asserts
//     that the inharmonic aliasing floor in the output spectrum is rejected
//     below a defined threshold (and that the 1x baseline genuinely aliases,
//     proving the test exercises the alias path rather than being a no-op).
//   * OversamplingEngine PDC            : asserts the exact integer latency
//     reported by getLatencySamples() for the 1x, 2x and 4x factors, both
//     against the deterministic expected literals and against an independent
//     reference juce::dsp::Oversampling constructed with identical design
//     parameters (filterHalfBandFIREquiripple, integer latency).
//   * AutoGainBypass loudness matching   : a smoke check that, after RMS
//     tracking converges, bypassing the wet path reproduces the wet
//     loudness on the dry signal (output RMS ~= wet RMS).
//
// Tests are auto-registered with juce::UnitTest and discovered by the
// juce::UnitTestRunner launched from tests/TestMain.cpp.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Source/DSP/OversamplingEngine.h"
#include "Source/DSP/AutoGainBypass.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 64;
constexpr int kNumChannels = 1;
constexpr double kTwoPi = 6.283185307179586476925286766559;

// A cubic soft-clip nonlinearity generates a dominant 3rd harmonic (and
// fundamental) only, so the alias analysis is unambiguous: at a 10 kHz
// fundamental the 3rd harmonic (30 kHz) sits above the 24 kHz base-rate
// Nyquist and folds back as an inharmonic alias in a 1x path, while a 4x
// path synthesises it cleanly in the 192 kHz domain and the half-band
// reconstruction filter rejects it on the way down.
struct SoftClipper
{
    float process (float x) const noexcept
    {
        return x - 0.3f * x * x * x;
    }
};

// Runs the full upsample -> nonlinearity -> downsample round trip for a sine
// probe of the given frequency through the prepared engine, capturing the
// resulting base-rate samples into `capture`.
void runOversampledProbe (ShredBoost::OversamplingEngine& engine,
                          double frequency,
                          double amplitude,
                          int totalSamples,
                          std::vector<float>& capture)
{
    capture.resize (static_cast<std::size_t> (totalSamples), 0.0f);
    juce::AudioBuffer<float> inputBuffer (kNumChannels, kBlockSize);
    juce::AudioBuffer<float> outputBuffer (kNumChannels, kBlockSize);

    int sample = 0;
    while (sample < totalSamples)
    {
        const int n = std::min (kBlockSize, totalSamples - sample);

        for (int i = 0; i < n; ++i)
        {
            const double s = amplitude
                * std::sin (kTwoPi * frequency * static_cast<double> (sample + i) / kSampleRate);
            inputBuffer.setSample (0, i, static_cast<float> (s));
        }
        // Zero the tail of the last (possibly partial) block.
        for (int i = n; i < kBlockSize; ++i)
            inputBuffer.setSample (0, i, 0.0f);

        juce::dsp::AudioBlock<const float> inBlock (inputBuffer);
        auto upsampled = engine.upsample (inBlock);

        SoftClipper clipper;
        for (std::size_t ch = 0; ch < upsampled.getNumChannels(); ++ch)
        {
            auto* data = upsampled.getChannelPointer (ch);
            for (std::size_t i = 0; i < upsampled.getNumSamples(); ++i)
                data[i] = clipper.process (data[i]);
        }

        for (int ch = 0; ch < kNumChannels; ++ch)
            std::fill (outputBuffer.getWritePointer (ch),
                       outputBuffer.getWritePointer (ch) + kBlockSize,
                       0.0f);

        juce::dsp::AudioBlock<float> outBlock (outputBuffer);
        engine.downsample (outBlock);

        for (int i = 0; i < n; ++i)
            capture[static_cast<std::size_t> (sample + i)] = outputBuffer.getSample (0, i);

        sample += n;
    }
}

// Computes the magnitude spectrum (linear, per-bin) of the given real signal
// using a Hann window and a juce::dsp::FFT of the next power-of-two size <= the
// capture length. Returns the bin magnitudes and the effective bin width in Hz.
struct Spectrum
{
    std::vector<float> magnitudes;
    double binWidthHz = 0.0;
    double sampleRate = 0.0;
};

Spectrum computeMagnitudeSpectrum (const std::vector<float>& capture, double sampleRate)
{
    const auto fftOrder = static_cast<int> (std::floor (std::log2 (capture.size())));
    const auto fftSize = 1 << fftOrder;
    const std::size_t usable = static_cast<std::size_t> (fftSize);

    std::vector<std::complex<float>> input (usable, std::complex<float> { 0.0f, 0.0f });
    std::vector<std::complex<float>> output (usable, std::complex<float> { 0.0f, 0.0f });
    for (std::size_t i = 0; i < usable; ++i)
    {
        // Hann window
        const double w = 0.5 - 0.5 * std::cos (kTwoPi * static_cast<double> (i) / static_cast<double> (usable));
        input[i] = std::complex<float> { capture[i] * static_cast<float> (w), 0.0f };
    }

    juce::dsp::FFT fft (fftOrder);
    fft.perform (input.data(), output.data(), false);

    std::vector<float> mags (usable / 2, 0.0f);
    for (std::size_t k = 0; k < usable / 2; ++k)
    {
        const float re = output[k].real();
        const float im = output[k].imag();
        mags[k] = std::sqrt (re * re + im * im);
    }

    Spectrum s;
    s.magnitudes = std::move (mags);
    s.binWidthHz = sampleRate / static_cast<double> (fftSize);
    s.sampleRate = sampleRate;
    return s;
}

// Returns the largest magnitude (in dB relative to `referenceMagnitude) among
// bins whose frequency is NOT within toleranceHz of any harmonic of
// `fundamentalHz` (harmonics 1..maxHarmonic that fall below Nyquist).
double maxInharmonicAliasDb (const Spectrum& spectrum,
                             double fundamentalHz,
                             int maxHarmonic,
                             double toleranceHz,
                             double referenceMagnitude)
{
    const double nyquist = spectrum.sampleRate * 0.5;
    double maxAlias = 0.0;

    for (std::size_t k = 1; k < spectrum.magnitudes.size(); ++k)
    {
        const double freq = static_cast<double> (k) * spectrum.binWidthHz;
        if (freq >= nyquist)
            break;

        bool harmonic = false;
        for (int h = 1; h <= maxHarmonic; ++h)
        {
            const double harmonicFreq = fundamentalHz * static_cast<double> (h);
            if (harmonicFreq >= nyquist)
                break;
            if (std::abs (freq - harmonicFreq) <= toleranceHz)
            {
                harmonic = true;
                break;
            }
        }

        if (! harmonic && spectrum.magnitudes[k] > maxAlias)
            maxAlias = spectrum.magnitudes[k];
    }

    if (maxAlias <= 1.0e-12 || referenceMagnitude <= 1.0e-12)
        return -300.0;

    return 20.0 * std::log10 (maxAlias / referenceMagnitude);
}

double binMagnitudeDb (const Spectrum& spectrum, double targetHz, double toleranceHz, double referenceMagnitude)
{
    const double nyquist = spectrum.sampleRate * 0.5;
    double maxMag = 0.0;

    for (std::size_t k = 1; k < spectrum.magnitudes.size(); ++k)
    {
        const double freq = static_cast<double> (k) * spectrum.binWidthHz;
        if (freq >= nyquist)
            break;
        if (std::abs (freq - targetHz) <= toleranceHz && spectrum.magnitudes[k] > maxMag)
            maxMag = spectrum.magnitudes[k];
    }

    if (maxMag <= 1.0e-12 || referenceMagnitude <= 1.0e-12)
        return -300.0;

    return 20.0 * std::log10 (maxMag / referenceMagnitude);
}

// Finds the strongest bin magnitude (used as the reference / fundamental).
double peakMagnitude (const Spectrum& spectrum)
{
    double peak = 0.0;
    for (const auto m : spectrum.magnitudes)
        if (m > peak)
            peak = m;
    return peak;
}

// Constructs an independent reference oversampler with the exact design
// parameters the engine uses, so PDC can be asserted against the literal
// integer latency the underlying design produces.
int referenceLatencyForFactor (int factor)
{
    using OS = juce::dsp::Oversampling<float>;
    const int factorIndex = (factor == 1) ? 0 : (factor == 2 ? 1 : 2);

    OS reference (static_cast<std::size_t> (kNumChannels),
                  static_cast<std::size_t> (factorIndex),
                  OS::filterHalfBandFIREquiripple,
                  true,
                  true);
    reference.initProcessing (static_cast<std::size_t> (kBlockSize));
    return juce::roundToInt (reference.getLatencyInSamples());
}

} // namespace

// =============================================================================
// OversamplingEngine alias rejection
// =============================================================================
class OversamplingAliasTests final : public juce::UnitTest
{
public:
    OversamplingAliasTests()
        : juce::UnitTest ("OversamplingEngine alias rejection", "Oversampling")
    {
    }

    void runTest() override
    {
        beginTest ("4x oversampling rejects inharmonic alias floor below -60 dB");

        // 10 kHz fundamental, hard-clipped at 0.5. The 3rd harmonic (30 kHz)
        // and above sit above the 24 kHz base-rate Nyquist, so a 1x path
        // folds them back as inharmonic aliases (8 kHz, 18 kHz, ...). A 4x
        // path synthesises them in the 192 kHz domain and the half-band
        // reconstruction filter rejects them on the way down.
        constexpr double frequency = 10000.0;
        constexpr double amplitude = 1.0;
        constexpr int totalSamples = 1 << 15; // 32768

        // ---- 4x path: alias floor must be well below the threshold ----
        ShredBoost::OversamplingEngine engine4x;
        engine4x.prepare (kSampleRate, kBlockSize, kNumChannels);
        engine4x.setOversamplingFactor (4);

        std::vector<float> capture4x;
        runOversampledProbe (engine4x, frequency, amplitude, totalSamples, capture4x);

        const auto spectrum4x = computeMagnitudeSpectrum (capture4x, kSampleRate);
        const double ref4x = peakMagnitude (spectrum4x);
        const double aliasDb4x = maxInharmonicAliasDb (spectrum4x, frequency, 8, 150.0, ref4x);

        // Sanity: the fundamental must actually be present and dominant.
        const double fundamentalDb4x = binMagnitudeDb (spectrum4x, frequency, 150.0, ref4x);
        expect (fundamentalDb4x > -1.0,
                "4x fundamental must be near 0 dB; got " + juce::String (fundamentalDb4x, 4) + " dB");

        // The behavioural claim: alias floor rejected below -60 dB.
        const double aliasThresholdDb = -60.0;
        expect (aliasDb4x < aliasThresholdDb,
                "4x oversampling must reject inharmonic alias floor below "
                    + juce::String (aliasThresholdDb, 1) + " dB; measured "
                    + juce::String (aliasDb4x, 4) + " dB");

        // ---- 1x baseline: must alias, proving the test is meaningful ----
        ShredBoost::OversamplingEngine engine1x;
        engine1x.prepare (kSampleRate, kBlockSize, kNumChannels);
        engine1x.setOversamplingFactor (1);

        std::vector<float> capture1x;
        runOversampledProbe (engine1x, frequency, amplitude, totalSamples, capture1x);

        const auto spectrum1x = computeMagnitudeSpectrum (capture1x, kSampleRate);
        const double ref1x = peakMagnitude (spectrum1x);
        const double aliasDb1x = maxInharmonicAliasDb (spectrum1x, frequency, 8, 150.0, ref1x);

        expect (aliasDb1x > -45.0,
                "1x baseline must produce a measurable inharmonic alias floor (>= -45 dB) "
                "to prove the oversampling test is exercising the alias path; measured "
                    + juce::String (aliasDb1x, 4) + " dB");
        expect (aliasDb4x < aliasDb1x - 15.0,
                "4x alias floor must be substantially lower than the 1x baseline "
                "(1x=" + juce::String (aliasDb1x, 3) + " dB, 4x=" + juce::String (aliasDb4x, 3) + " dB)");
    }
};

// =============================================================================
// OversamplingEngine exact PDC latency
// =============================================================================
class OversamplingPDCTests final : public juce::UnitTest
{
public:
    OversamplingPDCTests()
        : juce::UnitTest ("OversamplingEngine exact PDC latency", "Oversampling")
    {
    }

    void runTest() override
    {
        // Deterministic integer latencies produced by JUCE's
        // filterHalfBandFIREquiripple design with useIntegerLatency = true,
        // for the engine's 1x / 2x / 4x factors. These are the exact PDC
        // values the host must be told. They are cross-checked against an
        // independent reference oversampler built with identical parameters.
        struct FactorCase { int factor; int expectedLatency; };
        const FactorCase cases[] = {
            { 1, 0 },
            { 2, 49 },
            { 4, 61 },
        };

        for (const auto& c : cases)
        {
            beginTest ("getLatencySamples() reports exact PDC for factor "
                       + juce::String (c.factor) + "x");

            ShredBoost::OversamplingEngine engine;
            engine.prepare (kSampleRate, kBlockSize, kNumChannels);
            engine.setOversamplingFactor (c.factor);

            // The engine performs a 10 ms mute-fade handoff on the audio
            // thread before the newly prepared oversampler becomes active and
            // its latency is reported. Pump enough blocks through the
            // upsample/downsample path for the fade to complete.
            pumpAudioBlocks (engine, 32);

            const int reported = engine.getLatencySamples();

            // Exact integer match against the deterministic literal.
            expectEquals (reported, c.expectedLatency,
                          "Factor " + juce::String (c.factor)
                              + "x latency mismatch: expected "
                              + juce::String (c.expectedLatency)
                              + ", got " + juce::String (reported));

            // PDC correctness: the engine must forward exactly the latency
            // the underlying oversampling design produces (no off-by-one,
            // no rounding drift).
            const int reference = referenceLatencyForFactor (c.factor);
            expectEquals (reported, reference,
                          "Factor " + juce::String (c.factor)
                              + "x PDC must equal the reference oversampler latency ("
                              + juce::String (reference) + "), got "
                              + juce::String (reported));
            expectEquals (c.expectedLatency, reference,
                          "Hardcoded literal for factor " + juce::String (c.factor)
                              + "x must match the reference design latency");

            // getOversamplingFactor() must reflect the active factor.
            expectEquals (engine.getOversamplingFactor(), c.factor,
                          "Active factor should be " + juce::String (c.factor)
                              + "x after setOversamplingFactor()");
        }
    }

private:
    // Feeds `numBlocks` of a low-level sine through the engine's
    // upsample/downsample round trip so the mute-fade factor handoff can
    // complete on the audio thread.
    void pumpAudioBlocks (ShredBoost::OversamplingEngine& engine, int numBlocks)
    {
        juce::AudioBuffer<float> inputBuffer (kNumChannels, kBlockSize);
        juce::AudioBuffer<float> outputBuffer (kNumChannels, kBlockSize);

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < kBlockSize; ++i)
            {
                const double s = 0.25
                    * std::sin (kTwoPi * 1000.0 * static_cast<double> (b * kBlockSize + i) / kSampleRate);
                inputBuffer.setSample (0, i, static_cast<float> (s));
            }

            juce::dsp::AudioBlock<const float> inBlock (inputBuffer);
            auto upsampled = engine.upsample (inBlock);
            juce::ignoreUnused (upsampled);

            for (int ch = 0; ch < kNumChannels; ++ch)
                std::fill (outputBuffer.getWritePointer (ch),
                           outputBuffer.getWritePointer (ch) + kBlockSize,
                           0.0f);

            juce::dsp::AudioBlock<float> outBlock (outputBuffer);
            engine.downsample (outBlock);
        }
    }
};

// =============================================================================
// AutoGainBypass loudness matching (smoke check)
// =============================================================================
class AutoGainBypassLoudnessTests final : public juce::UnitTest
{
public:
    AutoGainBypassLoudnessTests()
        : juce::UnitTest ("AutoGainBypass loudness matching", "Oversampling")
    {
    }

    void runTest() override
    {
        beginTest ("Bypassed dry output matches wet loudness after RMS tracking");

        // dry = 0.5-amplitude sine; wet = dry * 2.0 (1.0 amplitude). After the
        // estimators converge, gainCompensation should approach ~2.0, so a
        // bypassed dry block is scaled up to match the wet loudness.
        constexpr double frequency = 220.0;
        constexpr double dryAmplitude = 0.5;
        constexpr double wetGain = 2.0;
        const int trackingBlocks = 2000; // ~2.7 s at 64-sample blocks
        const int bypassBlocks = 200;    // ~0.27 s; well past the 20 ms crossfade
        const int measureStartBlocks = 20; // skip crossfade + settling tail

        ShredBoost::AutoGainBypass bypass;
        bypass.prepare (kSampleRate, kBlockSize, kNumChannels);

        juce::AudioBuffer<float> dryBuffer (kNumChannels, kBlockSize);
        juce::AudioBuffer<float> wetBuffer (kNumChannels, kBlockSize);

        int sample = 0;

        // Tracking phase: feed wet = dry * wetGain with bypassed = false so
        // the estimators learn the dry/wet RMS ratio.
        for (int b = 0; b < trackingBlocks; ++b)
        {
            for (int i = 0; i < kBlockSize; ++i, ++sample)
            {
                const double s = dryAmplitude
                    * std::sin (kTwoPi * frequency * static_cast<double> (sample) / kSampleRate);
                dryBuffer.setSample (0, i, static_cast<float> (s));
                wetBuffer.setSample (0, i, static_cast<float> (s * wetGain));
            }

            juce::dsp::AudioBlock<const float> dryBlock (dryBuffer);
            juce::dsp::AudioBlock<float> wetBlock (wetBuffer);
            bypass.process (dryBlock, wetBlock, false);
        }

        const float trackedGain = bypass.getGainCompensation();
        expect (std::abs (trackedGain - static_cast<float> (wetGain)) < 0.15,
                "Tracked gain compensation should approach " + juce::String (wetGain, 3)
                    + " after convergence; got " + juce::String (trackedGain, 4));

        // Bypass phase: dry is scaled by gainCompensation and should match the
        // wet loudness. Measure the steady-state RMS of the bypassed output.
        double sumSqWet = 0.0;
        double sumSqBypassed = 0.0;
        int measured = 0;

        for (int b = 0; b < bypassBlocks; ++b)
        {
            for (int i = 0; i < kBlockSize; ++i, ++sample)
            {
                const double s = dryAmplitude
                    * std::sin (kTwoPi * frequency * static_cast<double> (sample) / kSampleRate);
                dryBuffer.setSample (0, i, static_cast<float> (s));
                // Wet continues to track so its RMS is meaningful to compare.
                wetBuffer.setSample (0, i, static_cast<float> (s * wetGain));
            }

            juce::dsp::AudioBlock<const float> dryBlock (dryBuffer);
            juce::dsp::AudioBlock<float> wetBlock (wetBuffer);
            bypass.process (dryBlock, wetBlock, true);

            if (b >= measureStartBlocks)
            {
                for (int i = 0; i < kBlockSize; ++i)
                {
                    sumSqBypassed += static_cast<double> (wetBuffer.getSample (0, i))
                        * static_cast<double> (wetBuffer.getSample (0, i));
                    sumSqWet += static_cast<double> (dryBuffer.getSample (0, i) * static_cast<float> (wetGain))
                        * static_cast<double> (dryBuffer.getSample (0, i) * static_cast<float> (wetGain));
                    ++measured;
                }
            }
        }

        expect (measured > 0, "Measurement window must contain samples");

        const double rmsBypassed = std::sqrt (sumSqBypassed / static_cast<double> (measured));
        const double rmsWet = std::sqrt (sumSqWet / static_cast<double> (measured));

        // Loudness-matched: bypassed output RMS within ~1.5 dB of the wet RMS.
        const double dbDiff = 20.0 * std::log10 ((rmsBypassed + 1.0e-12) / (rmsWet + 1.0e-12));
        expectWithinAbsoluteError (dbDiff, 0.0, 1.5,
                                   "Bypassed dry output should be loudness-matched to the wet signal "
                                   "(wet RMS=" + juce::String (rmsWet, 5)
                                       + ", bypassed RMS=" + juce::String (rmsBypassed, 5)
                                       + ", diff=" + juce::String (dbDiff, 3) + " dB)");
    }
};

// Static registrations make these tests discoverable by juce::UnitTestRunner.
static OversamplingAliasTests sOversamplingAliasTests;
static OversamplingPDCTests sOversamplingPDCTests;
static AutoGainBypassLoudnessTests sAutoGainBypassLoudnessTests;
