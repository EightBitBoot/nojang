@echo off

set WARN_DISABLE="/wd4710"
set DEFINES="/D_CRT_SECURE_NO_WARNINGS"

pushd bin
echo Compiling:
cl /nologo /diagnostics:caret /Zi /Wall /WX %WARN_DISABLE% %DEFINES% ..\nojang.c
popd

