# Additional options for the sanitizer
#
#export ASAN_OPTIONS=detect_leaks=1
export LSAN_OPTIONS=suppressions=${SOURCE}/tests/suppress-leaks.txt
