@echo off
cl /Fe:bin_to_strliteral.exe bin_to_strliteral.c
bin_to_strliteral.exe --extern --ident gui_font_binary_ttf ClearSans-Regular.ttf gui_font_binary_ttf.cpp
cl /std:c++20 /EHsc /MD /c plotlib.cpp gui_font_binary_ttf.cpp

link /DLL /OUT:libplotlib.dll plotlib.obj gui_font_binary_ttf.obj gdi32.lib msvcrt.lib raylib\libraylib_5_5_windows.lib user32.lib shell32.lib winmm.lib

del bin_to_strliteral.exe gui_font_binary_ttf.cpp gui_font_binary_ttf.obj plotlib.obj libplotlib.exp 
