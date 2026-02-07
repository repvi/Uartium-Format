cmake -S . -B build -G Ninja
cmake --build build -- -j 8
ctest --test-dir build --output-on-failure