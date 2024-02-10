#!/bin/sh
docker run  --rm -v "$PWD:/src" haydenkow/nu_dckos-beta make -j5 -f Makefile.dc $1 $2 $3 $4 $5