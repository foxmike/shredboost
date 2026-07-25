# Phase F Integration Verification

This document verifies the full processor integration delivered in **Phase F** of the
**ShredBoost** plugin. Phase F wires the DSP engines that were developed and unit-tested in
earlier phases into the live `processBlock` / `prepareToPlay` path of
`ShredBoostAudioProcessor`, and adds a real-time safety audit that proves the audio thread
performs no heap allocations and is free of denormal penalties.

It covers two areas:

1. **Integration** — how `ShredBoostEngine`, `OversamplingEngine`, and `AutoGainBypass` are
   wired into `processBlock`; how buffers are pre-allocated in `prepareToPlay`; the lock-free
   double-buffered parameter snapshot; the placement of `ScopedNoDenormals`; and how
   plugin-delay-compensation (PDC) / oversampling factor changes are handled on the message
   thread.
2. **Real-time audit** — what `tests/ProcessBlockRealtimeTests.cpp` verifies and the exact
   commands to reproduce it.

## 1. Phase F Deliverables

Phase F produces the following integration artifacts:

- **Integrated `processBlock` / `prepareToPlay`** — `Source/PluginProcessor.cpp`:
  the single real-time entry point that runs the full DSP chain (task f.1).
- **Real-time safety audit** — `tests/ProcessBlockRealtimeTests.cpp`:
  a pure `juce::UnitTest` that audits the integrated `processBlock` for zero heap
  allocations and denormal flushing (task f.2).
- **Build wiring** — `CMakeLists.txt` compiles `ProcessBlockRealtimeTests.cpp` into the
  `ShredBoostUnitTests` executable and registers it as the `ProcessBlockRealtimeTests`
  CTest entry.

## 2. Engine Integration into `processBlock`

`ShredBoostAudioProcessor::processBlock` (and the double-precision overload that delegates
to the float path via `doublePrecisionScratch`) is the only real-time entry point. The float
path is implemented in `processFloatBlock`, which drives the engines in the following order:

### 2.1 Parameter snapshot acquisition

Before touching any audio, the audio thread takes a coherent snapshot of all parameters:

```cpp
const auto targets = acquireParameterValues();
parameterSnapshot.setTargets (targets);
const auto values = parameterSnapshot.advance (numberOfSamples);
```

`acquireParameterValues()` reads the lock-free double-buffered snapshot published by the
message thread (see §4). `parameterSnapshot.advance(numberOfSamples)` produces the smoothed
parameter values for this block. All per-stage setters (`setTrimDb`, `setCutoff`,
`setFrequency`, `setAmount`, `setPickAttack`, `setVoicingMode`, `setTone`) are then called
from these smoothed values, so no parameter smoothing happens inside the engines
themselves.

### 2.2 Dry copy and per-channel pre-oversampling stages

The dry signal is copied into `dryBuffer` (pre-allocated in `prepareToPlay`) before any
processing, so `AutoGainBypass` can later mix dry against wet. Per-channel input trim,
tight filter, and mid voicing filter run on the base-band signal:

```cpp
inputTrim.processBlock (samples, numberOfSamples);
tightFilter.processBlock (samples, numberOfSamples);
midFilter.processBlock (samples, numberOfSamples);
```

### 2.3 `OversamplingEngine` (upsample)

The wet block is handed to the oversampler, which returns an oversampled sub-block:

```cpp
auto oversampledBlock = oversamplingEngine.upsample (constWetBlock);
```

The engine performs zero-latency or phase-compensated oversampling depending on the
selected factor and exposes `getLatencySamples()` for PDC (see §5). If the returned block
is empty the path clears the wet block and returns early — a defensive guard against
misconfiguration.

### 2.4 Drive gain ramp and `ShredBoostEngine`

The drive gain is ramped across the oversampled block via `applyGainRamp` (which updates
`currentDriveGain` in place), then the per-factor `ShredBoostEngine` processes the
oversampled signal. One `ShredBoostEngine` instance is prepared per oversampling factor
(`1×`, `2×`, `4×`) so each runs at its native sample rate with correctly sized internal
buffers:

```cpp
const auto factorIndex = oversamplingFactorToIndex (oversamplingEngine.getOversamplingFactor());
auto& engine = shredBoostEngines[static_cast<std::size_t> (factorIndex)];
engine.setPickAttack (values.pick_attack);
engine.setVoicingMode (values.voicing_mode);
engine.setTone (values.tone);
engine.process (oversampledBlock);
```

The engine index is derived from the *current* oversampling factor reported by the
oversampler, so it always matches the actually-active factor even mid-transition.

### 2.5 `OversamplingEngine` (downsample) and output gain

```cpp
oversamplingEngine.downsample (wetBlock);
applyGainRamp (wetBlock, currentOutputGain, juce::Decibels::decibelsToGain (values.level));
```

The output level is ramped across the base-band wet block.

### 2.6 `AutoGainBypass`

Finally the dry and wet blocks are combined by `AutoGainBypass`, which applies automatic
gain compensation when engaged and otherwise passes the wet signal through:

```cpp
autoGainBypass.process (constDryBlock, wetBlock, values.auto_gain);
```

`AutoGainBypass` is prepared in `prepareToPlay` for up to `maximumChannels` and
`preparedMaximumBlockSize`, so its internal buffers never resize on the audio thread.

## 3. Buffer Allocation in `prepareToPlay`

All allocations are confined to `prepareToPlay`, which is called by the host on a
non-real-time thread. The relevant allocations are:

- `dryBuffer.setSize (maximumChannels, preparedMaximumBlockSize, ...)` — the dry-signal
  copy used by `AutoGainBypass`.
- `doublePrecisionScratch.setSize (maximumChannels, preparedMaximumBlockSize, ...)` — the
  float scratch used by the double-precision `processBlock` overload.
- `oversamplingEngine.prepare (sampleRate, preparedMaximumBlockSize, maximumChannels)` —
  allocates the oversampler's internal polyphase buffers once.
- `autoGainBypass.prepare (sampleRate, preparedMaximumBlockSize, maximumChannels)` —
  allocates its gain estimator buffers once.
- `shredBoostEngines[i].prepare (sampleRate * factor, preparedMaximumBlockSize * factor)`
  for each of the `numberOfOversamplingFactors` instances — each engine is sized for its
  native rate and block size so it never reallocates when the active factor changes.
- Per-channel `inputTrimStages`, `tightFilters`, and `midVoicingFilters` are each
  `prepare`d at the base sample rate.

`preparedMaximumBlockSize` is captured as `std::max(1, samplesPerBlock)` and is used as
the bound in `processFloatBlock`: any block larger than `preparedMaximumBlockSize` is
cleared and returned untouched rather than triggering a reallocation. This guarantees
that the audio thread never grows a buffer.

## 4. Lock-Free Double-Buffered Parameter Snapshot

The host updates APVTS parameters on the message thread, but the audio thread must read
all parameters atomically and without locking. This is implemented with a
double-buffered snapshot:

- `parameterBuffers[0]` and `parameterBuffers[1]` each hold a full `ParameterValues`
  struct (all 11 parameters, including the `auto_gain` boolean).
- `publishedParameterBuffer` is a `std::atomic<int>` index selecting the currently
  published buffer.
- `parameterBufferReaders[0]` / `parameterBufferReaders[1]` are `std::atomic<unsigned int>`
  reader counters, enabling a small lock-free RCU-style handover.

`static_assert` guarantees the atomics in use are lock-free:

```cpp
static_assert (std::atomic<float>::is_always_lock_free);
static_assert (std::atomic<int>::is_always_lock_free);
static_assert (std::atomic<unsigned int>::is_always_lock_free);
```

**Publish path** (`publishParameterValues`, called from `timerCallback` on the message
thread at 30 Hz): the writer picks the inactive buffer, checks its reader count is zero,
copies the raw parameter values into it, and then atomically publishes the new index. If
the target buffer still has readers, the publish is skipped (the audio thread keeps the
previous snapshot) — this is the no-lock, no-wait, no-stall contract.

**Acquire path** (`acquireParameterValues`, called on the audio thread): the reader
loads the published index, increments that buffer's reader counter, re-checks the
published index (retrying once if it changed under it), copies the buffer into
`lastAudioParameterValues`, and decrements the reader counter. On a double-handover it
falls back to the last successfully read values, so the audio thread never blocks and
never reads a buffer mid-write.

`parameterSnapshot.setTargets (targets)` followed by `parameterSnapshot.advance
(numberOfSamples)` turns the coherent snapshot into per-sample smoothed values for the
current block.

## 5. PDC / Oversampling Handling on the Message Thread

Oversampling factor changes and plugin latency are managed exclusively on the message
thread inside `timerCallback` (which runs at 30 Hz via `startTimerHz(30)`), never on the
audio thread:

```cpp
void ShredBoostAudioProcessor::timerCallback()
{
    publishParameterValues();

    const auto values = parameterBuffers[...publishedParameterBuffer.load(acquire)...];
    const auto requestedFactor = oversamplingChoiceToFactor (values.oversampling);

    if (requestedFactor != lastRequestedOversamplingFactor)
    {
        oversamplingEngine.requestFactorChange (requestedFactor);
        lastRequestedOversamplingFactor = requestedFactor;
    }

    if (oversamplingEngine.isFactorChangePending())
        oversamplingEngine.applyPendingFactor();

    const auto latency = oversamplingEngine.getLatencySamples();
    if (latency != lastReportedLatencySamples)
    {
        lastReportedLatencySamples = latency;
        setLatencySamples (latency);
    }
}
```

Key points:

- **Factor change request** — `OversamplingEngine::requestFactorChange` only stages a
  change; the audio thread continues to run at the old factor until
  `applyPendingFactor()` swaps it in. This keeps the audio thread free of allocation and
  of factor-index races.
- **Latency reporting (PDC)** — `OversamplingEngine::getLatencySamples()` returns the
  processing latency introduced by oversampling. When it changes, the processor calls
  `setLatencySamples(latency)` so the host can compensate its playback timeline. Initial
  latency is set at the end of `prepareToPlay`.
- **Engine selection matches active factor** — because the factor transition is applied
  on the message thread and the audio thread reads `oversamplingEngine.getOversamplingFactor()`
  to pick the `ShredBoostEngine` instance, the per-factor engine always matches the
  actually-active oversampling factor.

## 6. `ScopedNoDenormals` Placement

`ScopedNoDenormals` is the very first object constructed in both `processBlock` overloads,
before any audio is touched:

```cpp
void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);
    processFloatBlock (...);
}
```

`ScopedNoDenormals` enables the CPU's DAZ (denormals-are-zero) and FTZ (flush-to-zero) MXCSR
bits for the duration of the block on the audio thread. DAZ causes denormal *loads* to be
treated as zero; FTZ flushes denormal *results* to zero. Constructing it at the very top of
`processBlock` guarantees the entire DSP chain — input trim, filters, oversampler,
`ShredBoostEngine`, output gain, and `AutoGainBypass` — runs with denormals disabled, so
neither incoming denormal content nor internally generated decaying tails can trigger the
denormal performance penalty. The guard is destroyed at block exit, restoring the host's
MXCSR state.

## 7. Real-Time Audit (`tests/ProcessBlockRealtimeTests.cpp`)

`ProcessBlockRealtimeTests.cpp` is a pure `juce::UnitTest` (no third-party frameworks) that
audits the integrated `processBlock` for the two real-time-safety invariants:

### 7.1 Allocation audit — `testZeroHeapAllocationDuringProcessBlock`

This test proves the audio thread performs **zero heap allocations** during `processBlock`
after `prepareToPlay` has run.

Mechanism:

- The test replaces the global `operator new` / `operator new[]` / `operator delete` /
  `operator delete[]` for the whole test executable. The replacements are transparent
  passthroughs to `malloc`/`free` unless a thread-local `g_countingAllocations` flag is
  armed; when armed, they increment a thread-local `g_allocationCount`.
- A `ScopedAllocationCounter` guard arms counting on the *current* thread only, so
  allocations on the message/timer thread (e.g. the 30 Hz `timerCallback`) do not perturb
  the count. The audio thread is the audited thread.
- After `processor.prepareToPlay(48000.0, 256)`, a stereo buffer of the maximum block size
  is pre-allocated and a single uncounted warm-up `processBlock` call settles any lazy
  initialisation (e.g. first-pass oversampler setup).
- The audit window then runs **200** `processBlock` calls under the counter, varying the
  input content per call (a phase-shifted sinusoid) so the DSP work cannot be elided.
- The test asserts `g_allocationCount == 0`:

```cpp
expectEquals (g_allocationCount, 0,
              "processBlock must not allocate on the audio thread, but "
              + juce::String (g_allocationCount)
              + " heap allocation(s) occurred over "
              + juce::String (countedIterations)
              + " processBlock calls after prepareToPlay");
```

What this proves: every buffer the audio thread touches (`dryBuffer`,
`doublePrecisionScratch`, oversampler internals, per-oversampling-factor `ShredBoostEngine`
internals, `AutoGainBypass` internals) was pre-allocated in `prepareToPlay` and never grows
on the audio thread.

### 7.2 Denormal audit — `testDenormalInputsAreFlushedToZero`

This test proves `ScopedNoDenormals` keeps the audio thread free of denormal penalties. It
has two sub-cases:

**(a) Subnormal input → exactly zero output.** Every sample of the input buffer is set to
`std::numeric_limits<float>::denorm_min()` (the smallest positive subnormal). After
`processBlock`, the test classifies every output sample with `std::fpclassify` and asserts:

```cpp
expectEquals (subnormalOutputs, 0,
              "processBlock output must contain no subnormal values when fed denormal input");
expectEquals (nonZeroOutputs, 0,
              "processBlock must flush denormal inputs to exactly zero output (ScopedNoDenormals DAZ/FTZ)");
```

DAZ makes the denormal loads read as zero, and FTZ flushes any denormal results to zero,
so the entire output is exactly `FP_ZERO`.

**(b) Decaying signal → no subnormal residuals.** A 1 kHz sinusoid with an exponential
decay (`exp(-0.72 * s)`, reaching ~1e-80 by the end of the block — deep into the
subnormal range) is fed in. The test asserts the output contains no `FP_SUBNORMAL`
samples, confirming FTZ flushes internally-generated denormals to zero so the tail does
not leave subnormal residuals:

```cpp
expectEquals (decaySubnormalOutputs, 0,
              "processBlock must flush internally-generated denormals to zero "
              "(no subnormal residuals in the output tail)");
```

## 8. Verification Commands

Run the following paste-and-run commands from the repository root. The JUCE submodule
must be populated (e.g. `git submodule update --init --recursive`) before configuring.

### Configure

```bash
cmake --preset linux-release
```

### Build

```bash
cmake --build build/linux-release --config Release
```

### Run the real-time safety audit (CTest)

The `ProcessBlockRealtimeTests` CTest entry runs the `ShredBoostUnitTests` executable,
which auto-discovers and runs the `ProcessBlockRealtimeTests` `juce::UnitTest` via the
shared `tests/TestMain.cpp` runner:

```bash
ctest --test-dir build/linux-release --build-config Release -R ProcessBlockRealtimeTests --output-on-failure
```

### Run the full test suite

```bash
ctest --test-dir build/linux-release --build-config Release --output-on-failure
```

## 9. Acceptance Checklist

- [ ] `cmake --preset linux-release` completes with exit code `0`.
- [ ] `cmake --build build/linux-release --config Release` completes with exit code `0`.
- [ ] `ctest --test-dir build/linux-release --build-config Release -R ProcessBlockRealtimeTests --output-on-failure`
      reports `ProcessBlockRealtimeTests` as passing.
- [ ] `ShredBoostEngine`, `OversamplingEngine`, and `AutoGainBypass` are all invoked from
      `processFloatBlock` in the order documented in §2.
- [ ] All audio-thread buffers are allocated in `prepareToPlay` and no allocation is
      performed during `processBlock` (allocation audit, §7.1).
- [ ] Denormal inputs and internally-generated denormals are flushed to zero
      (denormal audit, §7.2).
- [ ] Parameter reads on the audio thread use the lock-free double-buffered snapshot
      (§4) and never block.
- [ ] Oversampling factor changes and PDC latency updates are applied on the message
      thread in `timerCallback` (§5).
