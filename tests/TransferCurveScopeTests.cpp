// =============================================================================
// ShredBoost - TransferCurveScope UI Component Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Validates:
//   * Default weight values are correctly configured.
//   * Changing voicing mode updates target weights.
//   * Calling timerCallback smoothly transitions the current weights toward target weights.
//   * Pushing telemetry updates the instantaneous amplitude and smoothed amplitude.
//   * Binding external atomics allows polling of telemetry.
// =============================================================================

#include <juce_core/juce_core.h>
#include "Source/UI/TransferCurveScope.h"
#include <atomic>
#include <cmath>

class TransferCurveScopeTests : public juce::UnitTest
{
public:
    TransferCurveScopeTests()
        : juce::UnitTest ("TransferCurveScope Tests", "ShredBoost")
    {}

    void runTest() override
    {
        testDefaults();
        testVoicingModeTransitions();
        testPushTelemetry();
        testAtomicTelemetryBinding();
    }

private:
    void testDefaults()
    {
        beginTest ("Default states are correct");
        
        ShredBoost::TransferCurveScope scope;
        // Verify default target/current weights are for mode 0 (Yng-Grey)
        expectEquals (scope.targetWeights[0], 1.0f);
        expectEquals (scope.targetWeights[1], 0.0f);
        expectEquals (scope.targetWeights[2], 0.0f);
        expectEquals (scope.targetWeights[3], 0.0f);

        expectEquals (scope.currentWeights[0], 1.0f);
        expectEquals (scope.currentWeights[1], 0.0f);
        expectEquals (scope.currentWeights[2], 0.0f);
        expectEquals (scope.currentWeights[3], 0.0f);
        
        expectEquals (scope.smoothedAmplitude, 0.0f);
    }

    void testVoicingModeTransitions()
    {
        beginTest ("Voicing mode selection updates target weights and morphs current weights");
        
        ShredBoost::TransferCurveScope scope;
        scope.setVoicingMode (2);
        
        expectEquals (scope.targetWeights[0], 0.0f);
        expectEquals (scope.targetWeights[1], 0.0f);
        expectEquals (scope.targetWeights[2], 1.0f);
        expectEquals (scope.targetWeights[3], 0.0f);
        
        // Before timerCallback runs, currentWeights should still be old weights
        expectEquals (scope.currentWeights[0], 1.0f);
        
        // Call timerCallback to simulate transition progress
        scope.timerCallback();
        
        // Check that it began transitioning
        expect (scope.currentWeights[0] < 1.0f);
        expect (scope.currentWeights[2] > 0.0f);
        
        // Run timer callback many times to complete the morph transition
        for (int i = 0; i < 60; ++i)
            scope.timerCallback();
            
        // Weights should now be extremely close to the target
        expectWithinAbsoluteError (scope.currentWeights[2], 1.0f, 0.01f);
        expectWithinAbsoluteError (scope.currentWeights[0], 0.0f, 0.01f);
    }

    void testPushTelemetry()
    {
        beginTest ("Pushing telemetry updates internal state and smoothed position");

        ShredBoost::TransferCurveScope scope;
        scope.pushInputSample (0.5f);
        expectEquals (scope.instantaneousAmplitude.load(), 0.5f);

        // Run timerCallback to update smoothed amplitude
        scope.timerCallback();
        expectEquals (scope.smoothedAmplitude, 0.5f * 0.25f);
        
        // Let it run multiple times, it should converge to the target
        for (int i = 0; i < 30; ++i)
            scope.timerCallback();
            
        expectWithinAbsoluteError (scope.smoothedAmplitude, 0.5f, 0.01f);
    }

    void testAtomicTelemetryBinding()
    {
        beginTest ("Binding to external atomic variables polls correctly");

        ShredBoost::TransferCurveScope scope;
        std::atomic<float> amp { 0.75f };
        std::atomic<int> mode { 3 };

        scope.setTelemetrySources (&amp, &mode);
        
        // Execute timer callback to poll
        scope.timerCallback();
        
        expectEquals (scope.instantaneousAmplitude.load(), 0.75f);
        expectEquals (scope.targetWeights[3], 1.0f);
    }
};

static TransferCurveScopeTests sTransferCurveScopeTests;
