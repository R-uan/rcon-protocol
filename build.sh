#!/bin/bash

if [ ! -d "./build/" ]; then
	mkdir build
fi

cmake -S . -B ./build -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build ./build

echo "Project built"
