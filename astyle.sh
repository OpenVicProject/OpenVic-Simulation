#!/usr/bin/env bash

echo "\n\nFormatting openvic-simulation with astyle:\n"
astyle --options=.astylesrc --recursive ./src/*.?pp

exit 0
