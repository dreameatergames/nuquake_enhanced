# nuQuake Enhanced © 2024 HaydenKow/David Croshaw
## Free and Open for the Community!

## Working:
- The Game
- 16-bit Sound and CDDA Audio
- Rendering
- Keyboard + Mouse 
- Controllers (analog too!)
- Console (with keyboard)
- some mods
- saves

__let me know if stuff is broken.__

## Known Issues:
- Graphical glitches due to gldc hack
- Demos might not load
- Some Mods Crash
- No Networking
## Compiling Instructions
### Prerequisites
- Kallistios(KOS) Toolchain Installation in default install location
### KOS Setup
- ```sudo nano environ.sh```
- Under "Debug Builds" uncomment ```export KOS_CFLAGS="${KOS_CFLAGS} -DNDEBUG"```
- ```source environ.sh```
- ```make clean```
- ```make```
### Compiling
- ```make -f Makefile_quake.dc```
- if recompiling ```rm -rf build_dc/```

## Shoutouts
- Mrneo240
  - Coding everything
  - Putting up with me
- Kazade
  - GLdc: the best damn opengl implemention on the Dreamcast
  - Numerous discussions and info sharing
- Bruceleet
  - Fixing the rendering code
  - Debugging the main crash
- Ian Michael
  - SDL Sound Code and SDL Library
  - Inspiration and Encouragement
  - Tossing the save code over
  - BERO
  - Most of the input code (All old, had to be forward ported)
- The Dreamcast Community as a whole!
  - Simulant and DC Wiki Discord Servers, Assembler, DC-Talk, DCEMU



## How to run
- it runs CD-R or CD-ROM, dcload-ip with this directory structure:
```
\CD
  IP.BIN
  1ST_READ.BIN
  +- ID1
       +- PAK0.PAK
```


## Developer info
```
dcload-ip:
  same as burned image but on pc harddrive.

if quake directory is at C:\QUAKE, under cygwin enviroment, 
$ mount -b 'C:\quake' /quake
and
$ dcload-ip -t <dreamcast ip> glquake.elf
```
