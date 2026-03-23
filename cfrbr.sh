#!/bin/bash
./cfr0 $1 $2 $3
./cfr 0 64 $1 $2 $3
rm str$1$2$30.bin
./brorg 1 64 $1 $2 $3
./brorg 2 64 $1 $2 $3