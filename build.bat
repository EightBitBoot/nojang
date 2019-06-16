@echo off

set WARNS_DISABLE=/wd4710
set DEFINES=/D_CRT_SECURE_NO_WARNINGS
set LIBRARIES=WS2_32.lib

IF NOT EXIST "bin\" mkdir bin

pushd bin
echo Compiling:
cl /nologo /diagnostics:caret /Zi %WARNS_DISABLE% %DEFINES% ..\nojang.c /link %LIBRARIES%
popd

