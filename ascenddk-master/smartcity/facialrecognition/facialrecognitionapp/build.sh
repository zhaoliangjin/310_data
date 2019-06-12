#!/bin/sh
export LD_LIBRARY_PATH=$DDK_HOME/uihost/lib/
TOP_DIR=${PWD}
cd ${TOP_DIR} && make
cp graph.config ./out/
for file in ${TOP_DIR}/*
do
if [ -d "$file" ]
then
  if [ -f "$file/Makefile" ];then
    cd $file && make install
  fi
fi
done
