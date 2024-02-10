# nuQuake © 2024 David Croshaw
## Free and Open for the Community!

## Working:
- the game
- audio (+CD Audio)
- rendering
- keyboard+mouse 
- controllers (analog too!)
- console (with keyboard)
- some mods
- saves

__let me know if stuff is broken.__

## Known Issues:
- some mods crash
- no networking

## Shoutouts
- Hayden Kowalchuck
  - For the initial port of nuQuake
  - Putting up with so much of my BS
- Kazade
  - GLdc: the best damn opengl implemention on the Dreamcast
  - Numerous discussions and info sharing
- BERO
  - Sound code, and most of the input code (All old, had to be forward ported)
- Ian Michael
  - Inspiration and Encouragement
  - Tossing the save code over
- The Dreamcast Community as a whole!
  - Assembler, DC-Talk, DCEMU. 



## How to run
- it runs CD-R or CD-ROM, dcload-ip with this directory structure:

### burned Quake sharware version:
```
\QUAKE_SW
  +- ID1
       +- PAK0.PAK
```

### Quake commercial version full install:
```
\QUAKE
  +- ID1
  |    +- PAK0.PAK
  |    +- PAK1.PAK
  +- ALIEN (optional)
  |    +- PAK0.PAK
  |    +- PAK1.PAK
  +- MOD (optional)
  |    +- PAK0.PAK
```

### Quake commercial version CD-ROM:
```
\Data
  +- ID1
       +- glquake
       +- PAK0.PAK
       +- PAK1.PAK
```

## Notes
- These are checked for quake files, in order but any can be used:

| Folder         | Normal Origin   |
| :------------- | :-------------- |
| "/cd/QUAKE"    | installed       |
| "/cd/QUAKE_SW" | shareware       |
| "/cd/data"     | official CD-ROM |
| "/pc/quake"    | debug           |
| "/pc/quake_sw" | debug           |

## Developer info

### Steps to Compile:

1. Download and Install WSL2 and Ubuntu

2. Install "Docker for Windows"

3. ``` sudo apt install docker.io ```

4. Turn on "WSL2 Support" in Docker

5. Clone this git repo into a folder

6. ``` make dc ```


```
dcload-ip:
  same as burned image but on pc harddrive.

if quake directory is at C:\QUAKE, under cygwin enviroment, 
$ mount -b 'C:\quake' /quake
and
$ dcload-ip -t <dreamcast ip> glquake.elf
```
