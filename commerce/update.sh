#!/bin/bash
./commerce map_out.txt map_out.txt < "params/$1.txt" > "maps/$1.svg"
eog "maps/$1.svg" &
