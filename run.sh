#! /usr/bin/env bash
which gcc-14 || brewsome
make clean && make && LD_LIBRARY_PATH=./raylib/raylib ./c_chess
