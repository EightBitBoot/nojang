@echo off

set Wildcard=*.h *.cpp *.inl *.c

echo.

echo TODOS FOUND:
echo -------

findstr -s -n -i -l "TODO" %Wildcard%