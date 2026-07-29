/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== FactoryPresets.xml ==================
static const unsigned char temp_binary_data_0[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!--\n"
"  ShredBoost 1.0 - Factory Presets\n"
"\n"
"  Each <Preset> element contains a complete plugin state serialized in the\n"
"  AudioProcessorValueTreeState / juce::ValueTree format produced by\n"
"  ShredBoostAudioProcessor::getStateInformation().\n"
"\n"
"  The inner <ShredBoostState> element is the root of a single plugin state:\n"
"    - the `schemaVersion` attribute mirrors ShredBoost::ParamID::kParameterSchemaVersion\n"
"    - each child element tag is a frozen parameter ID from ParameterIDs.h\n"
"    - the `value` attribute holds the parameter value:\n"
"        * float parameters (input_trim, tight_cut, mid_freq, mid_amount,\n"
"          drive, tone, level, pick_attack) store the raw float value\n"
"        * AudioParameterChoice parameters (voicing_mode, oversampling) store\n"
"          the choice index as a number\n"
"        * AudioParameterBool (auto_gain) stores 0 / 1\n"
"\n"
"  voicing_mode choices: 0 = Yng-Grey, 1 = Precision Tight, 2 = SD-1, 3 = Clean\n"
"  oversampling choices: 0 = 1x, 1 = 2x, 2 = 4x\n"
"-->\n"
"<FactoryPresets>\n"
"\n"
"  <!-- 1. Thrash Chug: aggressive high-gain rhythm, tight low end via the\n"
"       Precision Tight HPF voicing, strong drive, pronounced pick attack,\n"
"       mid voicing set for chug articulation. -->\n"
"  <Preset name=\"Thrash Chug\">\n"
"    <ShredBoostState schemaVersion=\"1\">\n"
"      <input_trim value=\"0.0\"/>\n"
"      <tight_cut value=\"280.0\"/>\n"
"      <mid_freq value=\"620.0\"/>\n"
"      <mid_amount value=\"3.0\"/>\n"
"      <drive value=\"24.0\"/>\n"
"      <tone value=\"6.5\"/>\n"
"      <level value=\"0.0\"/>\n"
"      <pick_attack value=\"5.0\"/>\n"
"      <voicing_mode value=\"1\"/>\n"
"      <oversampling value=\"1\"/>\n"
"      <auto_gain value=\"0\"/>\n"
"    </ShredBoostState>\n"
"  </Preset>\n"
"\n"
"  <!-- 2. Yngwie Lead: singing fast lead with the Yng-Grey clip voicing,\n"
"       smoother mids, moderate tight_cut, level-appropriate for lead. -->\n"
"  <Preset name=\"Yngwie Lead\">\n"
"    <ShredBoostState schemaVersion=\"1\">\n"
"      <input_trim value=\"0.0\"/>\n"
"      <tight_cut value=\"150.0\"/>\n"
"      <mid_freq value=\"720.0\"/>\n"
"      <mid_amount value=\"5.0\"/>\n"
"      <drive value=\"20.0\"/>\n"
"      <tone value=\"7.0\"/>\n"
"      <level value=\"3.0\"/>\n"
"      <pick_attack value=\"2.0\"/>\n"
"      <voicing_mode value=\"0\"/>\n"
"      <oversampling value=\"2\"/>\n"
"      <auto_gain value=\"0\"/>\n"
"    </ShredBoostState>\n"
"  </Preset>\n"
"\n"
"  <!-- 3. SD-1 Rhythm: SD-1 clip voicing with a mid-hump rhythm tone. -->\n"
"  <Preset name=\"SD-1 Rhythm\">\n"
"    <ShredBoostState schemaVersion=\"1\">\n"
"      <input_trim value=\"0.0\"/>\n"
"      <tight_cut value=\"180.0\"/>\n"
"      <mid_freq value=\"720.0\"/>\n"
"      <mid_amount value=\"6.0\"/>\n"
"      <drive value=\"16.0\"/>\n"
"      <tone value=\"6.0\"/>\n"
"      <level value=\"0.0\"/>\n"
"      <pick_attack value=\"2.0\"/>\n"
"      <voicing_mode value=\"2\"/>\n"
"      <oversampling value=\"1\"/>\n"
"      <auto_gain value=\"0\"/>\n"
"    </ShredBoostState>\n"
"  </Preset>\n"
"\n"
"  <!-- 4. Clean Tight Attack: Clean voicing with low drive, emphasizing the\n"
"       transient pick-attack shaper and low-end tightening. -->\n"
"  <Preset name=\"Clean Tight Attack\">\n"
"    <ShredBoostState schemaVersion=\"1\">\n"
"      <input_trim value=\"0.0\"/>\n"
"      <tight_cut value=\"220.0\"/>\n"
"      <mid_freq value=\"720.0\"/>\n"
"      <mid_amount value=\"2.0\"/>\n"
"      <drive value=\"3.0\"/>\n"
"      <tone value=\"8.0\"/>\n"
"      <level value=\"0.0\"/>\n"
"      <pick_attack value=\"7.0\"/>\n"
"      <voicing_mode value=\"3\"/>\n"
"      <oversampling value=\"1\"/>\n"
"      <auto_gain value=\"0\"/>\n"
"    </ShredBoostState>\n"
"  </Preset>\n"
"\n"
"</FactoryPresets>\n";

const char* FactoryPresets_xml = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x302ca782:  numBytes = 3381; return FactoryPresets_xml;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "FactoryPresets_xml"
};

const char* originalFilenames[] =
{
    "FactoryPresets.xml"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
