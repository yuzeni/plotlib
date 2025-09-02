#!/bin/bash
gcc bin_to_strliteral.c -o bin_to_strliteral
./bin_to_strliteral --volatile --ident gui_font_binary_ttf ClearSans-Regular.ttf gui_font_binary_ttf.cpp

g++ -fPIC -fvisibility=hidden -c -Wall -Wextra -shared plotlib.cpp gui_font_binary_ttf.cpp
g++ -shared -o libplotlib.so plotlib.o gui_font_binary_ttf.o -L"./raylib/" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
