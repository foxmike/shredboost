#include "ParameterLayout.h"
#include "ParameterIDs.h"

juce::AudioProcessorValueTreeState::ParameterLayout ShredBoost::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    namespace ParamID = ShredBoost::ParamID;
    constexpr int version = ParamID::kParameterSchemaVersion;

    // 1. input_trim (-18..+18 dB, default 0)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::input_trim, version },
        "Input Trim",
        juce::NormalisableRange<float> (-18.0f, 18.0f),
        0.0f,
        "dB"
    ));

    // 2. tight_cut (80..400 Hz, default 180) - skewed frequency parameter
    juce::NormalisableRange<float> tightCutRange (80.0f, 400.0f);
    tightCutRange.setSkewForCentre (180.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::tight_cut, version },
        "Tight Cut",
        tightCutRange,
        180.0f,
        "Hz"
    ));

    // 3. mid_freq (500..1200 Hz, default 720) - skewed frequency parameter
    juce::NormalisableRange<float> midFreqRange (500.0f, 1200.0f);
    midFreqRange.setSkewForCentre (720.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::mid_freq, version },
        "Mid Frequency",
        midFreqRange,
        720.0f,
        "Hz"
    ));

    // 4. mid_amount (0..+9 dB, default +4)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::mid_amount, version },
        "Mid Amount",
        juce::NormalisableRange<float> (0.0f, 9.0f),
        4.0f,
        "dB"
    ));

    // 5. drive (0..+36 dB, default +12)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::drive, version },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 36.0f),
        12.0f,
        "dB"
    ));

    // 6. tone (1..16 kHz, default 6.5) - skewed frequency parameter
    juce::NormalisableRange<float> toneRange (1.0f, 16.0f);
    toneRange.setSkewForCentre (6.5f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::tone, version },
        "Tone",
        toneRange,
        6.5f,
        "kHz"
    ));

    // 7. level (-24..+12 dB, default 0)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::level, version },
        "Level",
        juce::NormalisableRange<float> (-24.0f, 12.0f),
        0.0f,
        "dB"
    ));

    // 8. pick_attack (0..+9 dB, default +2)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::pick_attack, version },
        "Pick Attack",
        juce::NormalisableRange<float> (0.0f, 9.0f),
        2.0f,
        "dB"
    ));

    // 9. voicing_mode (AudioParameterChoice, 4 choices: Yng-Grey/Precision Tight/SD-1/Clean, default Yng-Grey index 0)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::voicing_mode, version },
        "Voicing Mode",
        juce::StringArray { "Yng-Grey", "Precision Tight", "SD-1", "Clean" },
        0
    ));

    // 10. oversampling (AudioParameterChoice, 3 choices: 1x/2x/4x, default 2x index 1)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::oversampling, version },
        "Oversampling",
        juce::StringArray { "1x", "2x", "4x" },
        1
    ));

    // 11. auto_gain (AudioParameterBool, default false)
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::auto_gain, version },
        "Auto Gain",
        false
    ));

    return layout;
}
