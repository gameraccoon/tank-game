#!/bin/bash

mkdir -p cmake-build-linux-ninja

pushd cmake-build-linux-ninja
	cmake .. --preset linux-ninja
popd
