#!/bin/bash

cmake -B build -S . --preset default
cd build
ninja
./capstone
