// =============================================================================
// ShredBoost - SpectrumOverlay UI Component Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Validates:
//   * SpectrumFIFO push and pull operations.
//   * Analyser windowing, FFT execution, and peak frequency bin detection.
//   * SpectrumOverlay sample rate mapping.
// =============================================================================

#include <juce_core/juce_core.h>
#include "Source/UI/SpectrumOverlay.h"
#include <cmath>

class SpectrumOverlayTests : public juce::UnitTest
{
public:
    SpectrumOverlayTests()
        : juce::UnitTest ("SpectrumOverlay Tests", "ShredBoost")
    {}

    void runTest() override
    {
        testFIFOPushPull();
        testAnalyserFFT();
        testSpectrumOverlayProperties();
    }

private:
    void testFIFOPushPull()
    {
        beginTest ("SpectrumFIFO push and pull operations");

        ShredBoost::SpectrumFIFO fifo;
        expectEquals (fifo.getNumReady(), 0);

        // Push some values
        std::array<float, 128> inputData;
        for (size_t i = 0; i < inputData.size(); ++i)
            inputData[i] = static_cast<float> (i) * 0.01f;

        fifo.push (inputData.data(), static_cast<int> (inputData.size()));
        expectEquals (fifo.getNumReady(), static_cast<int> (inputData.size()));

        std::array<float, 128> outputData;
        outputData.fill (0.0f);
        int numRead = fifo.pull (outputData.data(), static_cast<int> (outputData.size()));

        expectEquals (numRead, static_cast<int> (inputData.size()));
        for (size_t i = 0; i < inputData.size(); ++i)
            expectEquals (outputData[i], inputData[i]);

        expectEquals (fifo.getNumReady(), 0);
    }

    void testAnalyserFFT()
    {
        beginTest ("Analyser windowing and FFT execution");

        ShredBoost::SpectrumFIFO fifo;
        // Let's push 512 samples of a 1 kHz sine wave at 48 kHz sample rate
        std::array<float, 512> sineWave;
        const float sampleRate = 48000.0f;
        const float frequency = 1000.0f;
        for (int i = 0; i < 512; ++i)
        {
            sineWave[static_cast<size_t> (i)] = std::sin (2.0f * juce::MathConstants<float>::pi * frequency * static_cast<float> (i) / sampleRate);
        }

        fifo.push (sineWave.data(), 512);

        ShredBoost::SpectrumOverlay::Analyser analyser;
        expectEquals (analyser.samplesSinceLastFFT, 0);

        // Feed to analyser
        analyser.processFromFIFO (&fifo);

        // It should have executed the FFT and updated the averaged spectrum.
        // Let's find the bin with the maximum magnitude.
        float maxMag = -1.0f;
        int maxBin = -1;
        for (int k = 0; k <= 256; ++k)
        {
            const float mag = analyser.averagedSpectrum[static_cast<size_t> (k)];
            if (mag > maxMag)
            {
                maxMag = mag;
                maxBin = k;
            }
        }

        // Expected bin index: 1000 * 512 / 48000 = 10.667 -> bin 11 or 10.
        expect (maxBin == 10 || maxBin == 11);
        expect (maxMag > 0.05f); // Verify there is a clear peak
    }

    void testSpectrumOverlayProperties()
    {
        beginTest ("SpectrumOverlay properties");

        ShredBoost::SpectrumOverlay overlay;
        expectEquals (overlay.sampleRate, 44100.0);

        overlay.setSampleRate (48000.0);
        expectEquals (overlay.sampleRate, 48000.0);

        ShredBoost::SpectrumFIFO preFIFO, postFIFO;
        overlay.setFIFOs (&preFIFO, &postFIFO);
        expect (overlay.preFIFO == &preFIFO);
        expect (overlay.postFIFO == &postFIFO);
    }
};

static SpectrumOverlayTests sSpectrumOverlayTests;
