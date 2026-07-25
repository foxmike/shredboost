// =============================================================================
// ShredBoost - ProcessBlock Real-Time Safety Audit
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Audits the
// integrated processBlock from task f.1 for two real-time-safety invariants:
//
//   1. Allocation audit: after prepareToPlay (which is allowed to allocate),
//      repeated processBlock calls must perform ZERO heap allocations. A
//      thread-local allocation counter backed by replaced global
//      operator new/delete is installed around the processBlock calls and the
//      count is asserted to be zero. A single uncounted warm-up call settles
//      any lazy initialisation.
//
//   2. Denormal audit: feeding buffers whose samples decay into the denormal
//      (subnormal) range must produce outputs that are flushed to zero. This
//      confirms the ScopedNoDenormals guard at the top of processBlock (DAZ
//      treats denormal loads as zero, FTZ flushes denormal results to zero)
//      keeps the audio thread free of denormal penalties.
//
// The test is auto-registered with juce::UnitTest and discovered by the shared
// juce::UnitTestRunner launched from tests/TestMain.cpp.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Source/PluginProcessor.h"

#include <new>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <limits>

namespace
{

// Thread-local allocation state. Only the calling (audio) thread is audited;
// allocations on the message/timer thread do not perturb the count.
thread_local int g_allocationCount = 0;
thread_local bool g_countingAllocations = false;

// Scoped guard that arms counting on the current thread and resets the
// counter. Destruction disables counting so later setup code is transparent.
struct ScopedAllocationCounter
{
    ScopedAllocationCounter() noexcept
    {
        g_allocationCount = 0;
        g_countingAllocations = true;
    }

    ~ScopedAllocationCounter() noexcept { g_countingAllocations = false; }
};

constexpr double kSampleRate = 48000.0;
constexpr int kMaxBlockSize = 256;
constexpr int kChannels = 2;
constexpr float kTwoPi = 6.283185307179586476925286766559f;

} // namespace

// =============================================================================
// Global operator new/delete replacements.
//
// These replace the global allocation routines for the whole test executable so
// that ANY heap allocation performed by processBlock is observable. While no
// ScopedAllocationCounter is active on the current thread they are transparent
// passthroughs to the C allocator (malloc/free) and therefore do not alter the
// behaviour of the other unit tests linked into the same runner. Only the
// unaligned single/array forms are replaced; over-aligned allocations continue
// to use the implementation-provided aligned operator new, which is correct
// and does not route through these overloads.
// =============================================================================
void* operator new (std::size_t size)
{
    void* p = std::malloc (size);
    if (p == nullptr) throw std::bad_alloc();
    if (g_countingAllocations) ++g_allocationCount;
    return p;
}

void* operator new[] (std::size_t size)
{
    void* p = std::malloc (size);
    if (p == nullptr) throw std::bad_alloc();
    if (g_countingAllocations) ++g_allocationCount;
    return p;
}

void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }

// =============================================================================
class ProcessBlockRealtimeTests final : public juce::UnitTest
{
public:
    ProcessBlockRealtimeTests()
        : juce::UnitTest ("ProcessBlock real-time safety", "ShredBoost") {}

    void runTest() override
    {
        testZeroHeapAllocationDuringProcessBlock();
        testDenormalInputsAreFlushedToZero();
    }

private:
    // ----------------------------------------------------------------------
    // 1) Allocation audit: zero heap allocations during processBlock.
    // ----------------------------------------------------------------------
    void testZeroHeapAllocationDuringProcessBlock()
    {
        beginTest ("processBlock performs zero heap allocations after prepareToPlay");

        ShredBoostAudioProcessor processor;
        processor.prepareToPlay (kSampleRate, kMaxBlockSize);

        // Pre-allocate the rendering buffers before the audit window.
        juce::AudioBuffer<float> buffer (kChannels, kMaxBlockSize);
        juce::MidiBuffer midi;

        for (int ch = 0; ch < kChannels; ++ch)
            juce::FloatVectorOperations::fill (buffer.getWritePointer (ch),
                                                0.0f, kMaxBlockSize);

        // Uncounted warm-up: settle any lazy state (e.g. oversampler first-pass
        // setup) so the audit measures the steady-state real-time path only.
        processor.processBlock (buffer, midi);

        constexpr int kIterations = 200;
        int countedIterations = 0;

        // Audit window: every heap allocation on this thread is counted.
        {
            ScopedAllocationCounter counter;

            for (int i = 0; i < kIterations; ++i)
            {
                // Vary content so work cannot be elided. Writing into the
                // pre-allocated buffer does not allocate.
                const float phase = static_cast<float> (i) * 0.01f;
                for (int ch = 0; ch < kChannels; ++ch)
                {
                    auto* w = buffer.getWritePointer (ch);
                    for (int s = 0; s < kMaxBlockSize; ++s)
                        w[s] = std::sin (phase + static_cast<float> (s) * 0.05f);
                }

                processor.processBlock (buffer, midi);
                ++countedIterations;
            }
        }

        expectEquals (g_allocationCount, 0,
                      "processBlock must not allocate on the audio thread, but "
                      + juce::String (g_allocationCount)
                      + " heap allocation(s) occurred over "
                      + juce::String (countedIterations)
                      + " processBlock calls after prepareToPlay");
    }

    // ----------------------------------------------------------------------
    // 2) Denormal audit: denormal inputs are flushed to zero in the output.
    // ----------------------------------------------------------------------
    void testDenormalInputsAreFlushedToZero()
    {
        beginTest ("processBlock flushes denormal inputs to zero via ScopedNoDenormals");

        ShredBoostAudioProcessor processor;
        processor.prepareToPlay (kSampleRate, kMaxBlockSize);

        juce::AudioBuffer<float> buffer (kChannels, kMaxBlockSize);
        juce::MidiBuffer midi;

        // (a) Feed sub-normal float values (below FLT_MIN). With ScopedNoDenormals
        // active, DAZ treats denormal loads as zero and FTZ flushes denormal
        // results to zero; the entire output must be exactly zero.
        const float denormal = std::numeric_limits<float>::denorm_min();

        for (int ch = 0; ch < kChannels; ++ch)
            juce::FloatVectorOperations::fill (buffer.getWritePointer (ch),
                                                denormal, kMaxBlockSize);

        processor.processBlock (buffer, midi);

        int subnormalOutputs = 0;
        int nonZeroOutputs = 0;

        for (int ch = 0; ch < kChannels; ++ch)
        {
            const auto* r = buffer.getReadPointer (ch);
            for (int s = 0; s < kMaxBlockSize; ++s)
            {
                const int cls = std::fpclassify (r[s]);
                if (cls == FP_SUBNORMAL) ++subnormalOutputs;
                if (cls != FP_ZERO) ++nonZeroOutputs;
            }
        }

        expectEquals (subnormalOutputs, 0,
                      "processBlock output must contain no subnormal values when "
                      "fed denormal input");
        expectEquals (nonZeroOutputs, 0,
                      "processBlock must flush denormal inputs to exactly zero "
                      "output (ScopedNoDenormals DAZ/FTZ)");

        // (b) A signal that decays across the block from a normal amplitude down
        // into the denormal range must not leave subnormal residuals in the
        // output (FTZ flushes internally-generated denormals to zero).
        const float decayRate = 0.72f; // exp(-0.72 * 256) ~ 1e-80 -> deep denormal

        for (int ch = 0; ch < kChannels; ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            for (int s = 0; s < kMaxBlockSize; ++s)
                w[s] = std::exp (-decayRate * static_cast<float> (s))
                       * std::sin (kTwoPi * 1000.0f * static_cast<float> (s)
                                   / static_cast<float> (kSampleRate));
        }

        processor.processBlock (buffer, midi);

        int decaySubnormalOutputs = 0;

        for (int ch = 0; ch < kChannels; ++ch)
        {
            const auto* r = buffer.getReadPointer (ch);
            for (int s = 0; s < kMaxBlockSize; ++s)
                if (std::fpclassify (r[s]) == FP_SUBNORMAL)
                    ++decaySubnormalOutputs;
        }

        expectEquals (decaySubnormalOutputs, 0,
                      "processBlock must flush internally-generated denormals to "
                      "zero (no subnormal residuals in the output tail)");
    }
};

// Static registration makes the test discoverable by juce::UnitTestRunner.
static ProcessBlockRealtimeTests sProcessBlockRealtimeTests;
