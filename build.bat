@echo off

set WARN_DISABLE="/wd4710"
set DEFINES="/D_CRT_SECURE_NO_WARNINGS"

IF NOT EXIST "bin\" mkdir bin

pushd bin
echo Compiling:
cl /nologo /diagnostics:caret /Zi %WARN_DISABLE% %DEFINES% WS2_32.lib ..\nojang.c
popd

