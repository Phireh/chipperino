REM I can't find the magic cmake incantation to do this
cmake -Bbuild
cmake --build build
move Debug\* .
rmdir Debug
rmdir build /s /q
