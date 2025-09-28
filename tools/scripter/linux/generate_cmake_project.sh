#!/bin/bash

python3 tools/build/run_code_generation.py

mkdir -p cmake-build-linux-ninja

pushd cmake-build-linux-ninja
	cmake .. --preset linux-ninja
popd
