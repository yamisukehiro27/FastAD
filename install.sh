#!/bin/bash

./clean-build.sh release -DFASTAD_ENABLE_TEST=OFF
cd build/release
sudo ninja install -j6
