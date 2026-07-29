# CMake generated Testfile for 
# Source directory: /home/mike/dev/mike/music/plugins/shredboost
# Build directory: /home/mike/dev/mike/music/plugins/shredboost/build-vst3
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ShredBoostParameterTests]=] "/home/mike/dev/mike/music/plugins/shredboost/build-vst3/ShredBoostUnitTests")
set_tests_properties([=[ShredBoostParameterTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;148;add_test;/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;0;")
add_test([=[ProcessBlockRealtimeTests]=] "/home/mike/dev/mike/music/plugins/shredboost/build-vst3/ShredBoostUnitTests")
set_tests_properties([=[ProcessBlockRealtimeTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;149;add_test;/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;0;")
add_test([=[FilterTests]=] "/home/mike/dev/mike/music/plugins/shredboost/build-vst3/ShredBoostTests")
set_tests_properties([=[FilterTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;188;add_test;/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;0;")
add_test([=[WaveshaperTests]=] "/home/mike/dev/mike/music/plugins/shredboost/build-vst3/ShredBoostTests")
set_tests_properties([=[WaveshaperTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;189;add_test;/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;0;")
add_test([=[OversamplingTests]=] "/home/mike/dev/mike/music/plugins/shredboost/build-vst3/ShredBoostTests")
set_tests_properties([=[OversamplingTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;190;add_test;/home/mike/dev/mike/music/plugins/shredboost/CMakeLists.txt;0;")
subdirs("JUCE")
