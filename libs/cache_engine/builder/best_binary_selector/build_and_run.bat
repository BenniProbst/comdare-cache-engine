@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /O2 /W3 best_binary_selector.cpp best_binary_selector_main.cpp /Fe:best_binary_selector.exe
