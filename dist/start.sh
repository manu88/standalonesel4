qemu-system-x86_64 -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce -nographic -serial mon:stdio -m size=3G -hda sofa.img
