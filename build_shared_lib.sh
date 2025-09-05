#!/bin/bash
gcc bin_to_strliteral.c -o bin_to_strliteral
./bin_to_strliteral --extern --ident gui_font_binary_ttf ClearSans-Regular.ttf gui_font_binary_ttf.cpp

g++ -fPIC -fvisibility=hidden -ggdb -c -Wall -Wextra -shared plotlib.cpp gui_font_binary_ttf.cpp
g++ -shared -o libplotlib.so plotlib.o gui_font_binary_ttf.o -L"./raylib/" -lraylib_5_5_linux -lGL -lm -lpthread -ldl -lrt -lX11

rm bin_to_strliteral gui_font_binary_ttf.cpp gui_font_binary_ttf.o plotlib.o 
