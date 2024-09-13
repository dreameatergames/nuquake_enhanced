
# GLdc - Dev Fork

## Bleeding edge features are found here and as such, may not always work or be as stable as upstream

**Fork of GLdc official upstream which also lives on [Gitlab](https://gitlab.com/simulant/GLdc)**

This is a partial implementation of OpenGL 1.2 for the SEGA Dreamcast for use
with the KallistiOS SDK.

It began as a fork of libGL by Josh Pearson but has undergone a large refactor
which is essentially a rewrite.

The aim is to implement as much of OpenGL 1.2 as possible, and to add additional
features via extensions.

Things left to (re)implement:

 - Spotlights (Trivial)
 - Framebuffer extension (Trivial)
 - Texture Matrix (Trivial)
 
Things I'd like to do:

 - Use a clean "gl.h"
 - Define an extension for modifier volumes
 - Add support for point sprites
 - Optimise, add unit tests for correctness
 
# Special Thanks!

 - Massive shout out to Hayden Kowalchuk for diagnosing and fixing a large number of bugs while porting GL Quake to the Dreamcast. Absolute hero!  
