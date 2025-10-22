#!/bin/sh
# SPDX-License-Identifier: MIT
#/ Copyright (c) 2025 AppLovin. All rights reserved.

# Java + C++
./build.sh -J -x -b Release

# Python
python3 -mvenv venv
source venv/bin/activate
pip3 install lmdb pure-cdb humanize
sh axoncache/build_python_module.sh

# Test that all python modules are there
# python3 axoncache/bench.py

# python3 axoncache/bench.py
# Java
# ./build.sh -J -b Release

