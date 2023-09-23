@echo off
pushd bin
	Debug\UnitTests.exe
	if %ERRORLEVEL% NEQ 0 (
		exit 1
	)
popd