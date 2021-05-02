qemu-system-x86_64  -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce -nographic -serial mon:stdio -kernel ../dist/kernel -initrd program.bin
