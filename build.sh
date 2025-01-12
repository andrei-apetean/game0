#!/bin/sh

bin_dir="bin"

if [ ! -d $bin_dir ]; then
    echo "creating bin dir: $(bin_dir)"
    mkdir $bin_dir
fi

if [[ ! -f "external/xdg-shell.h" && ! -f "external/xdg-shell.c" ]]; then
    xdg_shell_xml=$(find /usr/share/wayland-protocols -name "xdg-shell.xml")

    echo "Found xdg shell xml: $xdg_shell_xml"

    if command -v wayland-scanner >/dev/null 2>&1; then
        wayland-scanner client-header "$xdg_shell_xml" external/xdg-shell.h
        wayland-scanner private-code "$xdg_shell_xml" external/xdg-shell.c
    else
        echo "Error: wayland-scanner not found. Please install libwayland-dev."
        exit 1
    fi
fi

echo "Compiling executable... "
gcc -g ./src/main.c -o ./$bin_dir/main -I./external -I$VULKAN_SDK/Include/ -L$VULKAN_SDK/lib/ -lvulkan -lwayland-client

if [ $? -eq 0 ]; then 
    echo "Done"
fi

if [ -f $bin_dir/main ]; then 
    chmod +x ./$bin_dir/main
fi

