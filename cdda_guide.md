
# nuQuake CDDA Creation (example/tutorial)

## Tools Needed
- isobuster
- lbacalc
- mds4dc
- mkisofs
- Download: [https://www.mediafire.com/file/h5onoqouwm4h7e5/Quake.7z/file](https://www.mediafire.com/file/h5onoqouwm4h7e5/Quake.7z/file)

## Things Needed
- Quake cd (physical or properly ripped image)
- nuQuake
- patience

## Getting the Audio
- open your cd or image in IsoBuster 
- ![https://i.imgur.com/6UYtC3p.png](https://i.imgur.com/6UYtC3p.png)
- Extract all the music as 2352 RAW
- ![https://i.imgur.com/PFLBhcO.png](https://i.imgur.com/PFLBhcO.png)

- music all the files, untouched into the music folder
- put your ID1 and other games/mods in the second data folder
- put the nuQuake 1ST_READ.BIN in the first data folder

## Using the Audio

- run ``build_image.bat``
- congrats, working mds/mdf with cdda
  - Notes: Might be off by one, add a dummy track first
  - example: [https://www.mediafire.com/file/iqyj6mpzyr35xal/Track_01.7z/file](https://www.mediafire.com/file/iqyj6mpzyr35xal/Track_01.7z/file)
