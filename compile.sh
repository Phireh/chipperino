#!/bin/sh

# I can't be bothered to deal with cmake's idiosyncrasies
cmake -Bbuild . && cmake --build build && rm -r build
