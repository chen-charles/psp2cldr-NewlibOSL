psp2cldr-NewlibOSL
===
psp2cldr Newlib OS Support Reference Implementation  

## Brief
Supplementary ELFs of [psp2cldr](https://github.com/chen-charles/psp2cldr) using [the custom toolchain](https://github.com/chen-charles/buildscripts) require a set of runtime libraries to be loaded into the target environment.  
`psp2cldr-NewlibOSL` intends to be a showcase of a functional implementation that provides the necessary operating system support routines to [psp2cldr-newlib](https://github.com/chen-charles/psp2cldr-newlib) and [pthread-embedded](https://github.com/chen-charles/pthread-embedded) to enable the full potential of supplementary ELFs.  

## psp2cldr "additional options"
* `--sysroot <path>`: filesystem root  

## Dependencies
* [psp2cldr](https://github.com/chen-charles/psp2cldr); the expected version/tag will be tracked in [CMakeLists.txt](CMakeLists.txt)  
* [target.h](target.h) to be modified to suite your choice of `libc.so` loaded into the target environment; by default, `target.h` will be updated to track [psp2cldr-newlib](https://github.com/chen-charles/psp2cldr-newlib)  

## License
[GPLv2](LICENSE), or (at your option) any later version  
