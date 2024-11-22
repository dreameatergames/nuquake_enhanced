
# 🎮 SDL_dreamcast.h Functions Guide

## 📚 Table of Contents
- [Introduction](#-introduction)
- [Video Functions](#-video-functions)
- [Event Handling](#-event-handling)
- [Audio Functions](#-audio-functions)
- [Usage Example](#-usage-example)

## 🚀 Introduction

The `SDL_dreamcast.h` header file provides a set of functions as SDL add-ons specifically for the Dreamcast. These functions allow for fine-tuned control over video settings, input handling, and audio behavior on the Dreamcast hardware.

## 🖥️ Video Functions

### Setting the Video Driver

```c
SDL_DC_SetVideoDriver(SDL_DC_driver value)
```

Call this function before `SDL_Init` to choose the SDL video driver for Dreamcast.

| Driver | Description |
|:------:|-------------|
| `SDL_DC_DMA_VIDEO` (default) | Fastest video driver using double buffer. All graphic access uses RAM, and `SDL_Flip` sends data to VRAM using DMA. |
| `SDL_DC_TEXTURED_VIDEO` | Uses hardware texture for scaling, allowing virtual resolutions. PVR textures are always 2^n (128x128, 256x128, 512x256, etc.). |
| `SDL_DC_DIRECT_VIDEO` | Direct buffer video driver. Potentially faster than DMA driver when not using double buffering. |

### Other Video Functions

```c
SDL_DC_SetWindow(int width, int height)  // For textured video only
SDL_DC_VerticalWait(SDL_bool value)      // Enable/disable vertical retrace wait
SDL_DC_ShowAskHz(SDL_bool value)         // Enable/disable 50/60Hz choice (PAL only)
SDL_DC_Default60Hz(SDL_bool value)       // Set default to 60Hz (PAL only)
```

## 🕹️ Event Handling

### Mapping Dreamcast Buttons to SDL Keys

```c
SDL_DC_MapKey(int joy, SDL_DC_button button, SDLKey key)
```

Map a Dreamcast button to an `SDLKey`. 

| Parameter | Description |
|:---------:|-------------|
| `joy` | Dreamcast joystick port number (0, 1, 2, or 3) |
| `button` | Dreamcast button to map |
| `key` | SDL key to map to |

#### 📊 Default Button Mappings

<details>
<summary>Click to expand default mappings</summary>

| Button | Port 0 | Port 1 | Port 2 | Port 3 |
|:------:|:------:|:------:|:------:|:------:|
| SDL_DC_START | SDLK_RETURN | SDLK_z | SDLK_v | SDLK_m |
| SDL_DC_A | SDLK_LCTRL | SDLK_e | SDLK_y | SDLK_o |
| SDL_DC_B | SDLK_LALT | SDLK_q | SDLK_r | SDLK_u |
| SDL_DC_X | SDLK_SPACE | SDLK_x | SDLK_b | SDLK_COMMA |
| SDL_DC_Y | SDLK_LSHIFT | SDLK_c | SDLK_n | SDLK_PERIOD |
| SDL_DC_L | SDLK_TAB | SDLK_1 | SDLK_4 | SDLK_8 |
| SDL_DC_R | SDLK_BACKSPACE | SDLK_2 | SDLK_5 | SDLK_9 |
| SDL_DC_LEFT | SDLK_LEFT | SDLK_a | SDLK_f | SDLK_j |
| SDL_DC_RIGHT | SDLK_RIGHT | SDLK_d | SDLK_h | SDLK_l |
| SDL_DC_UP | SDLK_UP | SDLK_w | SDLK_t | SDLK_i |
| SDL_DC_DOWN | SDLK_DOWN | SDLK_s | SDLK_g | SDLK_k |

</details>

### Input Device Emulation

```c
SDL_DC_EmulateKeyboard(SDL_bool value)  // Enable/disable keyboard emulation
SDL_DC_EmulateMouse(SDL_bool value)     // Enable/disable mouse emulation
```

> ⚠️ **Note:** These functions require `SDL_OpenJoystick` to be called first.

## 🔊 Audio Functions

### Custom Sound Buffer Management

```c
SDL_DC_SetSoundBuffer(void *buffer)     // Set custom internal sound buffer
SDL_DC_RestoreSoundBuffer(void)         // Reset to default sound buffer
```

> 💡 **Tip:** Using a custom sound buffer can improve performance by avoiding memory copying in the sound callback.

## 💻 Usage Example

Here's an example demonstrating how to use some of these Dreamcast-specific functions:

```c
#include "include/SDL.h"
#include "include/SDL_dreamcast.h"

int main(int argc, char *argv[]) {
    SDL_DC_SetVideoDriver(SDL_DC_DMA_VIDEO);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface *screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen) {
        fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_DC_VerticalWait(SDL_TRUE);
    SDL_DC_Default60Hz(SDL_TRUE);

    // Open joystick
    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    if (joystick) {
        // Custom button mapping
        SDL_DC_MapKey(0, SDL_DC_A, SDLK_SPACE);
        
        // Enable mouse emulation
        SDL_DC_EmulateMouse(SDL_TRUE);
    }

    // Main game loop
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = 0;
            }
        }

        // Your game logic here...

        SDL_Flip(screen);
    }

    SDL_JoystickClose(joystick);
    SDL_Quit();
    return 0;
}
```

This example demonstrates:
- Setting up the video mode with the DMA driver
- Opening a joystick and customizing button mapping
- Enabling mouse emulation
- A basic game loop with event handling

> 🛠️ **Remember:** Compile your code with the appropriate SDL libraries and Dreamcast-specific settings.
