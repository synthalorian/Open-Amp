# CMake generated Testfile for 
# Source directory: /home/synth/projects/openamp/dsp-core/tests
# Build directory: /home/synth/projects/openamp/build-tests/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[DSPUnitTests]=] "/home/synth/projects/openamp/build-tests/tests/dsp-unit-tests")
set_tests_properties([=[DSPUnitTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/synth/projects/openamp/dsp-core/tests/CMakeLists.txt;38;add_test;/home/synth/projects/openamp/dsp-core/tests/CMakeLists.txt;0;")
subdirs("../_deps/catch2-build")
