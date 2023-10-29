#/usr/bin/env bash

echo "\n\nFormatting openvic-simulation with clang-format:\n"
find ./src/ -iname *.hpp -o -iname *.cpp | xargs clang-format --verbose -i

exit 0
