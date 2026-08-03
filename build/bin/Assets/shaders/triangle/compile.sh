#!/bin/bash

FAILED=0

~/vulkan/1.4.341.1/x86_64/bin/glslang -V shader.vert -o vert.spv
if [ $? -ne 0 ]; then
    read -n 1 -s -r -p "Vertex shader failed..."
    echo
    FAILED=1
fi

~/vulkan/1.4.341.1/x86_64/bin/glslang -V shader.frag -o frag.spv
if [ $? -ne 0 ]; then
    read -n 1 -s -r -p "Fragment shader failed..."
    echo
    FAILED=1
fi

if [ $FAILED -eq 0 ]; then
    read -n 1 -s -r -p "Success..."
    echo
fi
