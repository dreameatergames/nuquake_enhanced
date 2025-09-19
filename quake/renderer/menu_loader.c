/*
 * Filename: d:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\src\dreamcast\menu_loader.c
 * Path: d:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\src\dreamcast
 * Created Date: Monday, September 16th 2019, 8:41:40 am
 * Author: Hayden Kowalchuk
 *
 * Copyright (c) 2019 HaydenKow
 */

// menu_loader.c -- used for loading mods and such

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _arch_dreamcast
#include <malloc.h>
#include <dc/pvr.h>
#include <sys/dirent.h>
#else
#include <dirent.h>
#endif

#include "quakedef.h"

void M_Print(int cx, int cy, char *str);
void M_PrintWhite(int cx, int cy, char *str);
void DrawQuad(float x, float y, float w, float h, float u, float v, float uw, float vh);
void BitmapLoad(char *path, GLuint *temp_tex);
void glScaleF(float x, float y, float z);
void GL_Set2D(void);

extern unsigned int char_texture;

static char gamecmd[7];
static char *gamedir = "id1";
static int current_page = 0;

#define ISDIR(ent) ((ent)->size < 0)
#define GAMES_PER_PAGE 12

typedef struct
{
  char gamename[32];
  char dirname[32];
  char cmdline[128];
  //char snapfile[32];
  //char txtfile[32];
  GLuint texture;
  void *next;
} game_t;

typedef struct
{
  game_t *games;
  int n_games;
} gamelist_t;

gamelist_t *gamelist;

game_t *get_game_object(int num) {
  if (!num) {
    return gamelist->games;
  }
  game_t *temp = gamelist->games;
  for (int i = 0; i < num; i++) {
    temp = (game_t *)temp->next;
  }
  return temp;
}

void free_gamelist(void) {
  if (gamelist == NULL)
    return;

  for (int i = gamelist->n_games - 1; i >= 0; i--) {
    game_t *temp = get_game_object(i);
    if (temp->texture) {
      if (glIsTexture(temp->texture))
        glDeleteTextures(1, &temp->texture);
    }
    free(temp);
  }
  free(gamelist);
}

static int load_mod_list(char *curdir) {
  game_t *cur_game;

  gamelist->games = malloc(sizeof(game_t));
  memset(gamelist->games, 0, sizeof(game_t));
  gamelist->n_games = 0;

  cur_game = gamelist->games;

  DIR *d;

  d = opendir(curdir);

  if (!d) {
    //printf("Error Opening (%s)!\n", curdir);
    return 0;
  }

  struct dirent *de;

  qboolean is_dir = false;
  while ((de = readdir(d)) != NULL) {
    if (!strcasecmp(de->d_name, ".") || !strcasecmp(de->d_name, "..")) {
      continue;
    }
    char dir_name[256];

    {  // the only method if d_type isn't available,
      // otherwise this is a fallback for FSes where the kernel leaves it DT_UNKNOWN.
      struct stat stbuf;
      sprintf(dir_name, "%s%s%s", curdir, "/", de->d_name);
      // stat follows symlinks, lstat doesn't.
      stat(dir_name, &stbuf);  // TODO: error check
      is_dir = S_ISDIR(stbuf.st_mode);
    }

    if (is_dir) {
      struct dirent *de2;
#if defined(_WIN32)
      sprintf(dir_name, "%s%s%s", curdir, "\\", de->d_name);
#else
      sprintf(dir_name, "%s%s%s", curdir, "/", de->d_name);
#endif
      DIR *dir = opendir(dir_name);
      de2 = NULL;
      while ((de2 = readdir(dir))) {
        char *ext = strrchr(de2->d_name, '.');
        if (ext == NULL)
          continue;
        ext++;
        //if ((!strcasecmp(ext, "pak") || (!strcasecmp(de2->d_name, "font.bmp"))))
        if (!strcasecmp(ext, "pak")) {
          if (gamelist->n_games >= 1) {
            cur_game->next = malloc(sizeof(game_t));
            memset(cur_game->next, 0, sizeof(game_t));
            cur_game = cur_game->next;
          } else {
            cur_game = gamelist->games;
          }
          strncpy(cur_game->dirname, de->d_name, 32);
          strncpy(cur_game->gamename, de->d_name, 32);
          cur_game->texture = 0;
          //cur_game->snapfile = NULL;
          //cur_game->txtfile = NULL;
          cur_game->next = NULL;

          /* Check for image to load */
          char image[256];
#ifdef _arch_dreamcast
          sprintf(image, "%s%s%s", dir_name, "/", "image.bmp");
#else
          sprintf(image, "%s%s%s", dir_name, "\\", "image.bmp");
#endif

          if (Sys_FileTime(image) == 1) {
            BitmapLoad(image, &cur_game->texture);
          }

          /* Check for cmdline arguments */
#ifdef _arch_dreamcast
          sprintf(image, "%s%s%s", dir_name, "/", "cmdline.txt");
#else
          sprintf(image, "%s%s%s", dir_name, "\\", "cmdline.txt");
#endif
          memset(cur_game->cmdline, '\0', sizeof(cur_game->cmdline));
          if (Sys_FileTime(image) == 1) {
            FILE *cmdfile = fopen(image, "r");
            fread(cur_game->cmdline, 128, 1, cmdfile);
            fclose(cmdfile);
          }

          gamelist->n_games++;
          break;
        }
      }
      closedir(dir);
    }
  }

  closedir(d);
  return gamelist->n_games;
}

static int mod_cursor = 0;
static int selected_mod = -1;

void M_Mod_Loader(int key) {
  switch (key) {
    case K_ALT:
    case K_SPACE:
    case K_ESCAPE:
      selected_mod = 0;
      break;

    case K_LEFTARROW:
      mod_cursor = 0;
      if (current_page)
        current_page--;
      break;

    case K_RIGHTARROW:
      mod_cursor = 0;
      if (gamelist->n_games - (current_page * GAMES_PER_PAGE) > GAMES_PER_PAGE) {
        current_page++;
      } else {
        mod_cursor = MIN(gamelist->n_games - (current_page * GAMES_PER_PAGE), GAMES_PER_PAGE) - 1;
      }
      break;

    case K_UPARROW:
      mod_cursor--;
      if (mod_cursor < 0) {
        if (current_page != 0) {
          mod_cursor = MIN(gamelist->n_games - 1, GAMES_PER_PAGE - 1);
          if (current_page)
            current_page--;
        } else {
          mod_cursor++;
        }
      }
      break;

    case K_DOWNARROW:
      mod_cursor++;
      if (mod_cursor >= MIN(gamelist->n_games - (current_page * GAMES_PER_PAGE), GAMES_PER_PAGE)) {
        if (mod_cursor >= GAMES_PER_PAGE && (gamelist->n_games - (current_page * GAMES_PER_PAGE) > GAMES_PER_PAGE)) {
          mod_cursor = 0;
          current_page++;
        } else {
          mod_cursor--;
        }
      }
      break;

    case K_ENTER:
      selected_mod = mod_cursor;
      break;
  }
}

/* storage for one texture  */
GLuint texture[1];

/* Image type - contains height, width, and data */
struct Image {
  uint32_t sizeX;
  uint32_t sizeY;
  char *data;
};
typedef struct Image Image;

// quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.
// See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/BMP.txt for more info.
int ImageLoad(char *filename, Image *image) {
  FILE *file;
  unsigned int size;         // size of the image in bytes.
  unsigned int i;            // standard counter.
  unsigned short int planes;  // number of planes in image (must be 1)
  unsigned short int bpp;     // number of bits per pixel (must be 24)
  char temp;                  // temporary color storage for bgr-rgb conversion.

  // make sure the file is there.
  if ((file = fopen(filename, "rb")) == NULL) {
    printf("File Not Found : %s\n", filename);
    return 0;
  }

  // seek through the bmp header, up to the width/height:
  fseek(file, 18, SEEK_CUR);

  // read the width
  if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
    printf("Error reading width from %s.\n", filename);
    return 0;
  }

  // read the height
  if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
    printf("Error reading height from %s.\n", filename);
    return 0;
  }

  // calculate the size (assuming 24 bits or 3 bytes per pixel).
  size = image->sizeX * image->sizeY * 3;

  // read the planes
  if ((fread(&planes, 2, 1, file)) != 1) {
    printf("Error reading planes from %s.\n", filename);
    return 0;
  }
  if (planes != 1) {
    printf("Planes from %s is not 1: %u\n", filename, planes);
    return 0;
  }

  // read the bpp
  if ((i = fread(&bpp, 2, 1, file)) != 1) {
    printf("Error reading bpp from %s.\n", filename);
    return 0;
  }
  if (bpp != 24) {
    printf("Bpp from %s is not 24: %u\n", filename, bpp);
    return 0;
  }

  // seek past the rest of the bitmap header.
  fseek(file, 24, SEEK_CUR);

  // read the data.
  image->data = (char *)malloc(size);
  if (image->data == NULL) {
    printf("Error allocating memory for color-corrected image data");
    return 0;
  }

  if ((i = fread(image->data, size, 1, file)) != 1) {
    printf("Error reading image data from %s.\n", filename);
    return 0;
  }
  fclose(file);

  for (i = 0; i < size; i += 3) {  // reverse all of the colors. (bgr -> rgb)
    temp = image->data[i];
    image->data[i] = image->data[i + 2];
    image->data[i + 2] = temp;
  }

  // we're done.
  return 1;
}

void BitmapLoad(char *path, GLuint *temp_tex) {
  // Load Texture
  Image *image1;
  void *ptr;

  // allocate space for texture
  if(posix_memalign(&ptr, 32, sizeof(Image)) != 0){
    printf("Error allocating space for image");
    exit(0);
  }
  image1 = (Image*)ptr;
  if (image1 == NULL) {
    printf("Error allocating space for image");
    exit(0);
  }
  if (!ImageLoad(path, image1)) {
    exit(1);
  }

  // Create Texture
  glGenTextures(1, temp_tex);
  glBindTexture(GL_TEXTURE_2D, *temp_tex);  // 2d texture (x and y size)

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image,
  // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
  free(image1->data);
  free(ptr);
}

/* floats for x rotation, y rotation, z rotation */
float xrot, yrot, zrot;

extern void MYgluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
void InitGL(int Width, int Height)  // We call this right after our OpenGL window is created.
{
//Load Textures
#ifdef _arch_dreamcast
  BitmapLoad("/cd/font.bmp", &texture[0]);
#else
  BitmapLoad("font.bmp", &texture[0]);
#endif

  glEnable(GL_TEXTURE_2D);
  glClearColor(0.0f, 0.5f, 0.5f, 0.0f);  // This Will Clear The Background Color To Black
  glClearDepth(1.0);                     // Enables Clearing Of The Depth Buffer

  glViewport(0, 0, Width, Height);  // Reset The Current Viewport And Perspective Transformation

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  // Reset The Projection Matrix

  //gluPerspective(45.0f, (GLfloat)Width / (GLfloat)Height, 0.1f, 100.0f);  // Calculate The Aspect Ratio Of The Window
  MYgluPerspective(45.0f, (GLfloat)Width / (GLfloat)Height, 0.1f, 100.0f);  // Calculate The Aspect Ratio Of The Window

  glMatrixMode(GL_MODELVIEW);
}

/* The main drawing function. */
void DrawGLScene() {
  //gamelist->n_games = 36;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear The Screen And The Depth Buffer
  glLoadIdentity();                                    // Reset The View

  glPushMatrix();
  GL_Set2D();
  glBindTexture(GL_TEXTURE_2D, texture[0]);

  float scale = 2.0f;
  glTranslatef(320 + (1 - scale * 320), 0, 0);
  glScaleF(scale, scale, 1);

  int y = TOP_MARGIN;
  static int flashing = 1;
  int pages = gamelist->n_games / GAMES_PER_PAGE + 1;
  char page[16 + 1] =
      "\x80"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x81"
      "\x82";
  int bar_length = sizeof(page) - 2 - 1;
  M_Print(LEFT_MARGIN, TOP_MARGIN, "Available Games:");

  for (int i = 0; i < MIN(gamelist->n_games - (current_page * GAMES_PER_PAGE), GAMES_PER_PAGE); i++) {
    char name[64];
    y += 12;

    game_t *current_game = get_game_object(i + (current_page * GAMES_PER_PAGE));
    sprintf(name,
            "%s"
            "%s",
            (mod_cursor == i) ? "\xd" : " ", current_game->dirname);


    //sprintf(name, "game entry %d", (i + 1) + (current_page * GAMES_PER_PAGE));

    if (mod_cursor == i) {
      if (flashing++ < 15) {
        M_Print(LEFT_MARGIN, y, name);
      } else {
        M_PrintWhite(LEFT_MARGIN, y, name);
      }
      flashing %= 30;

      //Draw image
      /*if (current_game->texture) {
        glBindTexture(GL_TEXTURE_2D, current_game->texture);
        DrawQuad(320, TOP_MARGIN, 128, 128, 0, 1, 1, -1);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
      }*/
    } else {
      M_PrintWhite(LEFT_MARGIN, y, name);
    }
  }

  if (pages > 1) {
    memcpy(page + 1 + (int)(((float)(bar_length / (float)pages) + 1) * current_page), "\x83", 1);
    M_PrintWhite(LEFT_MARGIN, 180, page);
  }
  glPopMatrix();

  GL_EndRendering();
}

static char cmdline[256];
char *menu(int *argc, char **argv, char **basedirs, int num_dirs) {
#ifdef _arch_dreamcast
  printf("PVR Mem left:%u\n", (unsigned int)pvr_mem_available());
#ifdef GL_EXT_dreamcast_yalloc
  printf("GL Mem left:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
#endif
  malloc_stats();
#endif

  gamelist = malloc(sizeof(gamelist_t));
  int i, found = 0;
  for (i = 0; i < num_dirs; i++) {
    printf("Checking %s\n", basedirs[i]);
    if (load_mod_list(basedirs[i])) {
      printf("STICKING WITH %s\n", basedirs[i]);
      found = i;
      break;
    }
  }

  if (gamelist->n_games > 1) {
  //if (true) {
    //Setup
    InitGL(640, 480);

    //Fake our window
    glx = gly = 0;
    glwidth = 640;
    glheight = 480;
    vid.width = glwidth;
    vid.height = glheight;

    key_dest = key_menu;
    m_state = m_mod_loader;

    //Enabled Needed things for text
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    while (selected_mod == -1) {
      //Draw
      DrawGLScene();

      //Message Pump
      Sys_SendKeyEvents();

      //Use Input
      IN_Commands();
    }
    glDeleteTextures(1, texture);
    char_texture = 0;
    key_dest = 0;
    m_state = 0;
  } else {
    selected_mod = 0;
    game_t *base = get_game_object(selected_mod);
    /* Check for cmdline arguments */
#ifdef _arch_dreamcast
    char cmd_file[128];
    sprintf(cmd_file, "%s%s%s", base->dirname, "/", "cmdline.txt");
#else
    char cmd_file[256];
    sprintf(cmd_file, "%s%s%s", base->dirname, "\\", "cmdline.txt");
#endif
  printf("Checking %s\n", cmd_file);
    memset(base->cmdline, '\0', sizeof(base->cmdline));
    if (Sys_FileTime(cmd_file) == 1) {
      FILE *cmdfile = fopen(cmd_file, "r");
      fread(base->cmdline, 128, 1, cmdfile);
      fclose(cmdfile);
    }
    printf("read: %s\n", base->cmdline);
  }

  memset(cmdline, 0, 256);

  gamedir = get_game_object(selected_mod)->dirname;
  memset(gamecmd, '\0', sizeof(gamecmd));
  if ((!strcasecmp(gamedir, "rogue")) || (!strcasecmp(gamedir, "hipnotic"))) {
    strcat(gamecmd, "-");
  } else if (!strcasecmp(gamedir, "id1")) {
    gamedir = " ";
  } else {
    strcat(gamecmd, "-game ");
  }

  sprintf(cmdline, "%s%s %s", gamecmd, gamedir, get_game_object(selected_mod)->cmdline);
  printf("CMD: %s\n", cmdline);

  i = *argc;
  {
    char *s = cmdline;
    while (*s) {
      argv[i++] = s;
      while (*s && *s != ' ')
        s++;
      if (*s == ' ') {
        *s++ = 0;
        while (*s == ' ')
          s++;
      }
    }
  }
  argv[i] = NULL;
  *argc = i;

  free_gamelist();

#ifdef _arch_dreamcast
  printf("PVR Mem left:%u\n", (unsigned int)pvr_mem_available());
#ifdef GL_EXT_dreamcast_yalloc
  printf("GL Mem left:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
#endif
  malloc_stats();
#endif

  return basedirs[found];
}
