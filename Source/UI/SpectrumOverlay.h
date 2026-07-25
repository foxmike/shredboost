#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class SpectrumOverlayTests;

namespace ShredBoost
{

//==============================================================================
/**
    A thread-safe, lock-free, single-producer single-consumer FIFO for streaming
    raw audio samples from the audio thread to the GUI thread.
*/
class SpectrumFIFO
{
public:
    SpectrumFIFO() : fifo (8192)
    {
        buffer.setSize (1, 8192);
    }

    ~SpectrumFIFO() = default;

    /** Pushes a block of mono samples. Audio thread safe (lock-free). */
    void push (const float* data, int numSamples) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

        if (size1 > 0)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (0, start1), data, size1);

        if (size2 > 0)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (0, start2), data + size1, size2);

        fifo.finishedWrite (size1 + size2);
    }

    /** Pulls samples into a destination buffer. GUI thread safe (lock-free). */
    int pull (float* dest, int maxSamples) noexcept
    {
        int start1, size1, start2, size2;
        const int numToRead = std::min (maxSamples, fifo.getNumReady());
        fifo.prepareToRead (numToRead, start1, size1, start2, size2);

        if (size1 > 0)
            juce::FloatVectorOperations::copy (dest, buffer.getReadPointer (0, start1), size1);

        if (size2 > 0)
            juce::FloatVectorOperations::copy (dest + size1, buffer.getReadPointer (0, start2), size2);

        fifo.finishedRead (size1 + size2);
        return size1 + size2;
    }

    int getNumReady() const noexcept
    {
        return fifo.getNumReady();
    }

    void reset() noexcept
    {
        fifo.reset();
        buffer.clear();
    }

private:
    juce::AbstractFifo fifo;
    juce::AudioBuffer<float> buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumFIFO)
};

//==============================================================================
/**
    An overlaid pre-drive and post-drive frequency spectrum visualizer component.
*/
class SpectrumOverlay : public juce::Component,
                        private juce::Timer
{
    friend class ::SpectrumOverlayTests;

public:
    SpectrumOverlay() noexcept;
    ~SpectrumOverlay() override;

    /** Binds the visualizer to external pre/post drive lock-free FIFOs. */
    void setFIFOs (SpectrumFIFO* preDriveFIFO, SpectrumFIFO* postDriveFIFO) noexcept;

    /** Updates the sample rate used to compute frequency bins. */
    void setSampleRate (double newSampleRate) noexcept;

    // Component overrides
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Analyser
    {
        Analyser() : fft (9)
        {
            const float halfN = 255.5f;
            float windowSum = 0.0f;
            for (int i = 0; i < 512; ++i)
            {
                const float term = (static_cast<float> (i) - halfN) / halfN;
                welchWindow[static_cast<size_t> (i)] = 1.0f - term * term;
                windowSum += welchWindow[static_cast<size_t> (i)];
            }
            fftNormalization = 2.0f / windowSum;

            averagedSpectrum.fill (0.0f);
            circularBuffer.fill (0.0f);
        }

        void processFromFIFO (SpectrumFIFO* fifo) noexcept;
        void performFFT() noexcept;
        void reset() noexcept;

        juce::dsp::FFT fft;
        std::array<float, 512> welchWindow {};
        float fftNormalization = 1.0f;
        std::array<float, 512> circularBuffer {};
        int writePointer = 0;
        int samplesSinceLastFFT = 0;
        static constexpr int hopSize = 256;
        std::array<float, 257> averagedSpectrum {};
    };

    float getInterpolatedMagnitude (const std::array<float, 257>& spectrum, float binIndex) const noexcept;
    float magnitudeToY (float mag, const juce::Rectangle<float>& plotBounds) const noexcept;

    SpectrumFIFO* preFIFO = nullptr;
    SpectrumFIFO* postFIFO = nullptr;

    double sampleRate = 44100.0;

    Analyser preAnalyser;
    Analyser postAnalyser;

    // Premium styling options
    juce::Colour preColour   { juce::Colour (0xffd000ff) }; // Neon magenta/purple
    juce::Colour postColour  { juce::Colour (0xff00e5ff) }; // Neon cyan
    juce::Colour gridColour  { juce::Colour (0xffffffff).withAlpha (0.07f) };
    juce::Colour backgroundColour { juce::Colour (0xff0b0e14) };

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumOverlay)
};

} // namespace ShredBoost
