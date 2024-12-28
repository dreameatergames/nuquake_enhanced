# nuQuake Enhanced © 2024 David Croshaw
## Free and Open for the Community!

## Working:
- The Game
- 16-bit Sound and CDDA Audio
- Rendering
- Keyboard + Mouse 
- Controllers (analog too!)
- Networking
- Console (with keyboard)
- Some mods


__let me know if stuff is broken.__

## Known Issues:
- Graphical glitches due to gldc hack
- CDDA audio might cut out under certain circumstances
- Demos might not load
- Some Mods Crash
- Saves not Working

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
- Ian Micheal
 -  Auto baseline config reading to set all quake functions
 -  Fix for muzzle flash and debugging for broken saving
  - Code cleanin warnings and fixes for sound and menu
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
## Credits
- David Croshaw
- Hayden Kowalchuck
- Ian Michael
- Maximqad
- Bruceleet
- Lobotomy
- Bero
- DarkAura
- Mankrip
- John Carmack
- Michael Abrash
- John Cash
- Dave 'Zoid' Kirsch
- Id Software