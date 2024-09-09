@echo off
set scriptDir=%~dp0

pushd %scriptDir:~0,-1%\..
	python tools/build/code_generate.py

	mkdir build\dedicated_server

	pushd build\dedicated_server
		cmake build ../.. -G "Visual Studio 17 2022" -DDEDICATED_SERVER=ON
	popd
popd
