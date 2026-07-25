// =============================================================================
// ShredBoost - Parameter Taxonomy & State Round-Trip Unit Tests
// =============================================================================
// Pure juce::UnitTest coverage (no third-party frameworks). Validates:
//   * createParameterLayout() yields exactly the 11 frozen parameter IDs with
//     the expected ranges, defaults, and choice sets.
//   * voicing_mode default is "Yng-Grey", oversampling default is "2x", and
//     auto_gain default is Off.
//   * ShredBoostAudioProcessor::getStateInformation / setStateInformation
//     round-trip restores every parameter value byte-for-byte, and the
//     serialized state embeds schemaVersion == 1.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Source/Parameters/ParameterIDs.h"
#include "Source/Parameters/ParameterLayout.h"
#include "Source/PluginProcessor.h"

namespace
{

namespace ParamID = ShredBoost::ParamID;

// Ordered list of the 11 frozen parameter IDs.
const juce::StringArray kExpectedIDs {
    ParamID::input_trim,
    ParamID::tight_cut,
    ParamID::mid_freq,
    ParamID::mid_amount,
    ParamID::drive,
    ParamID::tone,
    ParamID::level,
    ParamID::pick_attack,
    ParamID::voicing_mode,
    ParamID::oversampling,
    ParamID::auto_gain,
};

juce::RangedAudioParameter* findParameter (juce::AudioProcessor& processor, const juce::String& id)
{
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (ranged->getParameterID() == id)
                return ranged;

    return nullptr;
}

} // namespace

class ParameterTests : public juce::UnitTest
{
public:
    ParameterTests() : juce::UnitTest ("ShredBoost Parameter Tests", "ShredBoost") {}

    void runTest() override
    {
        testParameterLayoutTaxonomy();
        testVoicingOversamplingAutoGainDefaults();
        testStateRoundTrip();
    }

private:
    // ----------------------------------------------------------------------
    // (1) createParameterLayout() via the processor produces exactly the 11
    //     expected parameters with correct IDs, ranges, and defaults.
    // ----------------------------------------------------------------------
    void testParameterLayoutTaxonomy()
    {
        beginTest ("Parameter layout produces 11 parameters with correct IDs/ranges/defaults");

        ShredBoostAudioProcessor processor;

        const auto& params = processor.getParameters();
        expectEquals (params.size(), 11, "Expected exactly 11 parameters");

        // IDs present and in the canonical order.
        for (int i = 0; i < kExpectedIDs.size(); ++i)
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[i]);
            expect (ranged != nullptr, "Parameter " + juce::String (i) + " is not a RangedAudioParameter");
            expectEquals (ranged->getParameterID(), kExpectedIDs[i],
                          "Parameter ID mismatch at index " + juce::String (i));
        }

        // Float parameter ranges / defaults.
        struct FloatSpec
        {
            const char* id;
            float min, max, defaultValue;
        };

        const FloatSpec floatSpecs[] {
            { ParamID::input_trim,  -18.0f, 18.0f,  0.0f },
            { ParamID::tight_cut,    80.0f, 400.0f, 180.0f },
            { ParamID::mid_freq,    500.0f, 1200.0f, 720.0f },
            { ParamID::mid_amount,    0.0f, 9.0f,    4.0f },
            { ParamID::drive,         0.0f, 36.0f,  12.0f },
            { ParamID::tone,          1.0f, 16.0f,   6.5f },
            { ParamID::level,       -24.0f, 12.0f,   0.0f },
            { ParamID::pick_attack,   0.0f, 9.0f,    2.0f },
        };

        for (const auto& spec : floatSpecs)
        {
            auto* param = findParameter (processor, spec.id);
            expect (param != nullptr, "Missing parameter: " + juce::String (spec.id));

            // getDefaultValue() is public on AudioProcessorParameter but private
            // on the concrete AudioParameterFloat, so call it through the base
            // RangedAudioParameter interface.
            const auto& range = param->getNormalisableRange();
            expectEquals (range.start, spec.min, juce::String (spec.id) + " range start");
            expectEquals (range.end,   spec.max, juce::String (spec.id) + " range end");
            // getDefaultValue() is normalized 0..1; convert back to the real
            // parameter value to compare against the documented default.
            const auto denormalizedDefault = range.convertFrom0to1 (param->getDefaultValue());
            expectEquals (denormalizedDefault, spec.defaultValue,
                          juce::String (spec.id) + " default value");
        }

        // voicing_mode choice set + default index.
        {
            auto* param = findParameter (processor, ParamID::voicing_mode);
            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            expect (choice != nullptr, "voicing_mode is not an AudioParameterChoice");
            expectEquals (choice->choices.size(), 4, "voicing_mode should have 4 choices");
            expectEquals (choice->choices[0], juce::String ("Yng-Grey"), "voicing_mode choice 0");
            expectEquals (choice->choices[1], juce::String ("Precision Tight"), "voicing_mode choice 1");
            expectEquals (choice->choices[2], juce::String ("SD-1"), "voicing_mode choice 2");
            expectEquals (choice->choices[3], juce::String ("Clean"), "voicing_mode choice 3");
            expectEquals (choice->getIndex(), 0, "voicing_mode default index");
        }

        // oversampling choice set + default index.
        {
            auto* param = findParameter (processor, ParamID::oversampling);
            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            expect (choice != nullptr, "oversampling is not an AudioParameterChoice");
            expectEquals (choice->choices.size(), 3, "oversampling should have 3 choices");
            expectEquals (choice->choices[0], juce::String ("1x"), "oversampling choice 0");
            expectEquals (choice->choices[1], juce::String ("2x"), "oversampling choice 1");
            expectEquals (choice->choices[2], juce::String ("4x"), "oversampling choice 2");
            expectEquals (choice->getIndex(), 1, "oversampling default index (2x)");
        }

        // auto_gain bool + default Off (getDefaultValue() is private in
        // AudioParameterBool, so verify the default on a freshly-constructed
        // processor via its current value).
        {
            auto* param = findParameter (processor, ParamID::auto_gain);
            auto* boolParam = dynamic_cast<juce::AudioParameterBool*> (param);
            expect (boolParam != nullptr, "auto_gain is not an AudioParameterBool");
            expect (! boolParam->get(), "auto_gain default should be Off");
        }
    }

    // ----------------------------------------------------------------------
    // (2) Explicit default-string assertions for the discrete parameters.
    // ----------------------------------------------------------------------
    void testVoicingOversamplingAutoGainDefaults()
    {
        beginTest ("Discrete defaults: voicing_mode=Yng-Grey, oversampling=2x, auto_gain=Off");

        ShredBoostAudioProcessor processor;

        {
            auto* param = findParameter (processor, ParamID::voicing_mode);
            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            expect (choice != nullptr);
            expectEquals (choice->getCurrentChoiceName(), juce::String ("Yng-Grey"),
                          "voicing_mode default name");
        }

        {
            auto* param = findParameter (processor, ParamID::oversampling);
            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            expect (choice != nullptr);
            expectEquals (choice->getCurrentChoiceName(), juce::String ("2x"),
                          "oversampling default name");
        }

        {
            auto* param = findParameter (processor, ParamID::auto_gain);
            auto* boolParam = dynamic_cast<juce::AudioParameterBool*> (param);
            expect (boolParam != nullptr);
            expect (! boolParam->get(), "auto_gain default should be Off");
        }
    }

    // ----------------------------------------------------------------------
    // (3) getStateInformation / setStateInformation round-trip restores every
    //     parameter value, and the serialized state embeds schemaVersion == 1.
    // ----------------------------------------------------------------------
    void testStateRoundTrip()
    {
        beginTest ("APVTS state round-trip restores identical values with schema version 1");

        ShredBoostAudioProcessor source;

        // Drive every parameter to a non-default normalized value and record
        // the resulting host-visible (0..1) value the parameter actually took.
        juce::StringArray capturedIDs;
        juce::Array<float> capturedValues;

        const float injectedValues[] {
            0.25f, 0.83f, 0.5f, 0.17f, 0.71f,
            0.62f, 0.34f, 0.88f, 0.5f, 0.5f, 1.0f
        };

        for (int i = 0; i < kExpectedIDs.size(); ++i)
        {
            auto* param = findParameter (source, kExpectedIDs[i]);
            expect (param != nullptr, "Missing parameter during setup: " + kExpectedIDs[i]);
            param->setValueNotifyingHost (injectedValues[i]);

            capturedIDs.add (kExpectedIDs[i]);
            capturedValues.add (param->getValue());
        }

        // Serialize.
        juce::MemoryBlock saved;
        source.getStateInformation (saved);
        expect (saved.getSize() > 0, "getStateInformation produced empty data");

        // Assert the embedded schema version is 1 (and the state tree type).
        {
            const auto xml = juce::AudioProcessor::getXmlFromBinary (saved.getData(), (int) saved.getSize());
            expect (xml != nullptr, "Saved state is not valid XML");
            expectEquals (xml->getTagName(), juce::String ("ShredBoostState"),
                          "Saved state root tag");
            expect (xml->hasAttribute ("schemaVersion"), "Saved state missing schemaVersion");
            expectEquals (xml->getIntAttribute ("schemaVersion", -1),
                          ShredBoost::ParamID::kParameterSchemaVersion,
                          "Embedded schema version");
            expectEquals (xml->getIntAttribute ("schemaVersion", -1), 1,
                          "Schema version must equal 1");
        }

        // Restore into a fresh processor.
        ShredBoostAudioProcessor restored;
        restored.setStateInformation (saved.getData(), (int) saved.getSize());

        for (int i = 0; i < capturedIDs.size(); ++i)
        {
            auto* param = findParameter (restored, capturedIDs[i]);
            expect (param != nullptr, "Missing parameter after restore: " + capturedIDs[i]);
            expectEquals (param->getValue(), capturedValues[i],
                          "Restored value mismatch for " + capturedIDs[i]);
        }
    }
};

// Static registration makes the test discoverable by juce::UnitTestRunner.
static ParameterTests sParameterTests;
