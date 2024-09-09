#!/bin/bash
set -e

mkdir build/dedicated_server -p
pushd build/dedicated_server
	cmake ../.. -DDEDICATED_SERVER=ON -DBUILD_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
	cmake --build .
popd

mkdir package/dedicated_server -p
cp ./bin/DedicatedServer package/dedicated_server/
cp -rL resources package/dedicated_server/
rm -r package/dedicated_server/resources/animations
rm -r package/dedicated_server/resources/atlas
rm -r package/dedicated_server/resources/fonts
rm -r package/dedicated_server/resources/textures
